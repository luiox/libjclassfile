#include "jclassfile/io/class_file_writer.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

#include "jstr/conversion.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"
#include "jclassfile/model/constant/const_ref.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_double.hpp"
#include "jclassfile/model/constant/cp_dynamic.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_float.hpp"
#include "jclassfile/model/constant/cp_int.hpp"
#include "jclassfile/model/constant/cp_invoke_dynamic.hpp"
#include "jclassfile/model/constant/cp_long.hpp"
#include "jclassfile/model/constant/cp_method_handle.hpp"
#include "jclassfile/model/constant/cp_method_type.hpp"
#include "jclassfile/model/constant/cp_module.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
#include "jclassfile/model/constant/cp_package.hpp"
#include "jclassfile/model/constant/cp_string.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/constant_pool_constants.hpp"
#include "jclassfile/util/growing_byte_buffer.hpp"

namespace jclassfile::io {

using namespace jclassfile::model;

namespace {

bool isContinuationByte(ca::u8 byte)
{
    return (byte & 0xC0u) == 0x80u;
}

[[noreturn]] void throwEncodingError(const char* msg)
{
    throw InvalidClassException(msg);
}

void appendMutf8Unit(std::string& out, ca::u16 unit)
{
    if (unit == 0) {
        out.push_back(static_cast<char>(0xC0u));
        out.push_back(static_cast<char>(0x80u));
        return;
    }
    if (unit <= 0x7Fu) {
        out.push_back(static_cast<char>(unit));
        return;
    }
    if (unit <= 0x7FFu) {
        out.push_back(static_cast<char>(0xC0u | (unit >> 6)));
        out.push_back(static_cast<char>(0x80u | (unit & 0x3Fu)));
        return;
    }
    out.push_back(static_cast<char>(0xE0u | (unit >> 12)));
    out.push_back(static_cast<char>(0x80u | ((unit >> 6) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | (unit & 0x3Fu)));
}

ca::u32 decodeUtf8CodePoint(const std::string& text, std::size_t& off)
{
    const auto*       data = reinterpret_cast<const ca::u8*>(text.data());
    const std::size_t size = text.size();
    ca::u8            b0   = data[off];

    if ((b0 & 0x80u) == 0) {
        ++off;
        return b0;
    }
    if ((b0 & 0xE0u) == 0xC0u) {
        if (off + 1 >= size || !isContinuationByte(data[off + 1])) {
            throwEncodingError("Invalid UTF-8 sequence");
        }
        ca::u32 cp = ((b0 & 0x1Fu) << 6) | (data[off + 1] & 0x3Fu);
        if (cp < 0x80u) {
            throwEncodingError("Overlong UTF-8 sequence");
        }
        off += 2;
        return cp;
    }
    if ((b0 & 0xF0u) == 0xE0u) {
        if (off + 2 >= size || !isContinuationByte(data[off + 1]) ||
            !isContinuationByte(data[off + 2])) {
            throwEncodingError("Invalid UTF-8 sequence");
        }
        ca::u32 cp =
            ((b0 & 0x0Fu) << 12) | ((data[off + 1] & 0x3Fu) << 6) | (data[off + 2] & 0x3Fu);
        if (cp < 0x800u) {
            throwEncodingError("Overlong UTF-8 sequence");
        }
        off += 3;
        return cp;
    }
    if ((b0 & 0xF8u) == 0xF0u) {
        if (off + 3 >= size || !isContinuationByte(data[off + 1]) ||
            !isContinuationByte(data[off + 2]) || !isContinuationByte(data[off + 3])) {
            throwEncodingError("Invalid UTF-8 sequence");
        }
        ca::u32 cp = ((b0 & 0x07u) << 18) | ((data[off + 1] & 0x3Fu) << 12) |
                     ((data[off + 2] & 0x3Fu) << 6) | (data[off + 3] & 0x3Fu);
        if (cp < 0x10000u || cp > 0x10FFFFu) {
            throwEncodingError("Invalid UTF-8 code point");
        }
        off += 4;
        return cp;
    }

    throwEncodingError("Invalid UTF-8 sequence");
}

std::string encodeMutf8LikeJava(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (std::size_t off = 0; off < text.size();) {
        ca::u32 cp = decodeUtf8CodePoint(text, off);
        if (cp <= 0xFFFFu) {
            appendMutf8Unit(out, static_cast<ca::u16>(cp));
            continue;
        }

        cp -= 0x10000u;
        ca::u16 high = static_cast<ca::u16>(0xD800u + (cp >> 10));
        ca::u16 low  = static_cast<ca::u16>(0xDC00u + (cp & 0x3FFu));
        appendMutf8Unit(out, high);
        appendMutf8Unit(out, low);
    }
    if (out.size() > 0xFFFFu) {
        throw InvalidClassException("Modified UTF-8 entry too long");
    }
    return out;
}

}   // namespace

std::vector<ca::u8> ClassFileWriter::write(model::ClassFile& clazz)
{
    util::GrowingByteBuffer out;

    fallbackInstructionWriter_ = newFallbackInstructionWriter(clazz);
    attributeWriter_           = std::make_unique<AttributeWriter>(
        fallbackInstructionWriter_ ? fallbackInstructionWriter_.get() : nullptr);

    out.putInt(0xCAFEBABE);
    out.putShort(clazz.getVersionMinor());
    out.putShort(clazz.getVersionMajor());

    const ConstPool& pool = clazz.getPool();
    out.putShort(static_cast<int>(pool.size()));
    for (auto it = pool.iterator(); it.hasNext();) {
        const CpEntry* entry = it.next();
        writeCpEntry(out, entry);
    }

    out.putShort(clazz.getAccess());

    const CpClass* superClass = clazz.getSuperClass();
    out.putShort(clazz.getThisClass() ? clazz.getThisClass()->index() : 0);
    out.putShort(superClass ? superClass->index() : 0);

    const auto& interfaces = clazz.getInterfaceClasses();
    out.putShort(static_cast<int>(interfaces.size()));
    for (const CpClass* iface : interfaces) {
        out.putShort(iface ? iface->index() : 0);
    }

    const auto& fields = clazz.getFields();
    out.putShort(static_cast<int>(fields.size()));
    for (const Field* field : fields) {
        writeField(out, field);
    }

    const auto& methods = clazz.getMethods();
    out.putShort(static_cast<int>(methods.size()));
    for (const Method* method : methods) {
        writeMethod(out, method);
    }

    const auto& attributes = clazz.attributes();
    out.putShort(static_cast<int>(attributes.size()));
    for (const Attribute* attr : attributes) {
        writeAttribute(out, attr);
    }

    out.trim();
    return out.unwrap();
}

void ClassFileWriter::writeCpEntry(util::GrowingByteBuffer& out, const CpEntry* entry)
{
    if (!entry) return;
    CpTag tag = entry->tag();
    out.put(static_cast<int>(tag));
    switch (tag) {
    case CpTag::UTF8:
    {
        const auto& utf8 = static_cast<const CpUtf8&>(*entry);
        std::string text = encodeMutf8LikeJava(utf8.text());
        out.putShort(static_cast<int>(text.size()));
        for (char c : text) {
            out.put(static_cast<ca::u8>(c));
        }
        break;
    }
    case CpTag::INTEGER:
    {
        const auto& cpInt = static_cast<const CpInt&>(*entry);
        out.putInt(cpInt.value());
        break;
    }
    case CpTag::FLOAT:
    {
        const auto& cpFloat = static_cast<const CpFloat&>(*entry);
        out.putFloat(cpFloat.value());
        break;
    }
    case CpTag::LONG:
    {
        const auto& cpLong = static_cast<const CpLong&>(*entry);
        out.putLong(cpLong.value());
        break;
    }
    case CpTag::DOUBLE:
    {
        const auto& cpDouble = static_cast<const CpDouble&>(*entry);
        out.putDouble(cpDouble.value());
        break;
    }
    case CpTag::STRING:
    {
        const auto&   cpString = static_cast<const CpString&>(*entry);
        const CpUtf8* value    = cpString.value();
        out.putShort(value ? value->index() : 0);
        break;
    }
    case CpTag::CLASS:
    {
        const auto&   cpClass = static_cast<const CpClass&>(*entry);
        const CpUtf8* name    = cpClass.name();
        out.putShort(name ? name->index() : 0);
        break;
    }
    case CpTag::FIELD_REF:
    case CpTag::METHOD_REF:
    case CpTag::INTERFACE_METHOD_REF:
    {
        const auto& ref = static_cast<const ConstRef&>(*entry);
        out.putShort(ref.classRef() ? ref.classRef()->index() : 0);
        out.putShort(ref.nameType() ? ref.nameType()->index() : 0);
        break;
    }
    case CpTag::NAME_TYPE:
    {
        const auto& nt = static_cast<const CpNameType&>(*entry);
        out.putShort(nt.name() ? nt.name()->index() : 0);
        out.putShort(nt.type() ? nt.type()->index() : 0);
        break;
    }
    case CpTag::DYNAMIC:
    {
        const auto& dyn = static_cast<const CpDynamic&>(*entry);
        out.putShort(dyn.bsmIndex());
        out.putShort(dyn.nameType() ? dyn.nameType()->index() : 0);
        break;
    }
    case CpTag::METHOD_HANDLE:
    {
        const auto& mh = static_cast<const CpMethodHandle&>(*entry);
        out.put(mh.kind());
        out.putShort(mh.reference() ? mh.reference()->index() : 0);
        break;
    }
    case CpTag::METHOD_TYPE:
    {
        const auto& mt = static_cast<const CpMethodType&>(*entry);
        out.putShort(mt.descriptor() ? mt.descriptor()->index() : 0);
        break;
    }
    case CpTag::INVOKE_DYNAMIC:
    {
        const auto& id = static_cast<const CpInvokeDynamic&>(*entry);
        out.putShort(id.bsmIndex());
        out.putShort(id.nameType() ? id.nameType()->index() : 0);
        break;
    }
    case CpTag::MODULE:
    {
        const auto& mod = static_cast<const CpModule&>(*entry);
        out.putShort(mod.name() ? mod.name()->index() : 0);
        break;
    }
    case CpTag::PACKAGE:
    {
        const auto& pkg = static_cast<const CpPackage&>(*entry);
        out.putShort(pkg.packageName() ? pkg.packageName()->index() : 0);
        break;
    }
    default:
        throw InvalidClassException("Unknown constant-pool tag: " +
                                    std::to_string(static_cast<int>(tag)));
    }
}

void ClassFileWriter::writeField(util::GrowingByteBuffer& out, const Field* field)
{
    out.putShort(field->getAccess());
    out.putShort(field->name() ? field->name()->index() : 0);
    out.putShort(field->type() ? field->type()->index() : 0);
    const auto& attrs = field->attributes();
    out.putShort(static_cast<int>(attrs.size()));
    for (const Attribute* attr : attrs) {
        writeAttribute(out, attr);
    }
}

void ClassFileWriter::writeMethod(util::GrowingByteBuffer& out, const Method* method)
{
    out.putShort(method->getAccess());
    out.putShort(method->name() ? method->name()->index() : 0);
    out.putShort(method->type() ? method->type()->index() : 0);
    const auto& attrs = method->attributes();
    out.putShort(static_cast<int>(attrs.size()));
    for (const Attribute* attr : attrs) {
        writeAttribute(out, attr);
    }
}

void ClassFileWriter::writeAttribute(util::GrowingByteBuffer& out, const Attribute* attribute)
{
    std::vector<ca::u8> bytes = attributeWriter_->writeAttribute(*attribute);
    for (ca::u8 b : bytes) {
        out.put(b);
    }
}

}   // namespace jclassfile::io
