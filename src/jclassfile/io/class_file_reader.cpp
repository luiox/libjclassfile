// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/ClassFileReader.java
// 状态：阶段 4b 实现
// 备注：
//   - 阶段 4b：完整实现 read(byte[]) 主流程 + 2-pass pool reading + readField/readMethod。
//   - 阶段 4c：AttributeReader 是最小 stub（仅读 name + length + raw bytes，返回
//   DefaultAttribute）。
//   - 阶段 5：替换为完整 AttributeReader（含 38 个 attribute 分发 + 版本/上下文/EOF/dup 决策）。
//   - 阶段 5：替换 getFallbackInstructionReader() 为真实 FallbackInstructionReader。
//   - 阶段 8：构造 IllegalRewritingInstructionsReader + IllegalStrippingTransformer。
//   - 上游 cffb89ff：带 ACC_MODULE 的类声明了 superclass/interface/field/method 时
//     降级为普通类（清 ACC_MODULE、丢模块元数据属性、清理 CpModule/CpPackage）。
//   - 异常模型：原版 throw IOException / InvalidClassException；C++ 只用 InvalidClassException
//     作为对外契约（reader 内部用 std::runtime_error / std::logic_error 表达编程错误）。

#include "class_file_reader.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

#include "attribute_context.hpp"
#include "attribute_holder_type.hpp"
#include "attribute_reader.hpp"
#include "libca/core/datatype.hpp"
#include "jclassfile/io/mutf8_codec.hpp"
#include "jclassfile/model/attribute/attribute_constants.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/version_constants.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"
#include "jclassfile/model/constant/const_ref.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_double.hpp"
#include "jclassfile/model/constant/cp_dynamic.hpp"
#include "jclassfile/model/constant/cp_field_ref.hpp"
#include "jclassfile/model/constant/cp_float.hpp"
#include "jclassfile/model/constant/cp_int.hpp"
#include "jclassfile/model/constant/cp_interface_method_ref.hpp"
#include "jclassfile/model/constant/cp_invoke_dynamic.hpp"
#include "jclassfile/model/constant/cp_long.hpp"
#include "jclassfile/model/constant/cp_method_handle.hpp"
#include "jclassfile/model/constant/cp_method_ref.hpp"
#include "jclassfile/model/constant/cp_method_type.hpp"
#include "jclassfile/model/constant/cp_module.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
#include "jclassfile/model/constant/cp_package.hpp"
#include "jclassfile/model/constant/cp_string.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/constant/placeholders.hpp"
#include "jclassfile/model/field.hpp"
#include "jclassfile/model/instruction/cp_ref_instruction.hpp"
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/method.hpp"

namespace jclassfile::io {

using namespace jclassfile::model;

namespace {

constexpr ca::u32 MAGIC = 0xCAFEBABE;

[[noreturn]] void throwInvalid(const char* msg)
{
    throw InvalidClassException(msg);
}

[[noreturn]] void throwInvalid(const std::string& msg)
{
    throw InvalidClassException(msg);
}

std::string readMutf8AsUtf8(IndexableByteStream&                        is,
                            const ClassFileReader::LenientTextDecoder* lenient)
{
    const auto ref = is.readMutf8Ref();
    if (lenient == nullptr) return detail::decodeMutf8LikeJava(ref);
    // 宽容口径（D40 反馈 bug2）：字节不是合法 modified-UTF-8（实战中中文混淆
    // 器把 GBK 窄字节写进常量池）时交回调规范化，不让整个 class 拒读。
    try {
        return detail::decodeMutf8LikeJava(ref);
    }
    catch (const std::exception&) {
        return (*lenient)(std::string_view(ref.data(), ref.size()));
    }
}

const CpClass* asCpClass(const CpEntry* e)
{
    if (e == nullptr || e->tag() != CpTag::CLASS) return nullptr;
    return static_cast<const CpClass*>(e);
}

const CpUtf8* asCpUtf8(const CpEntry* e)
{
    if (e == nullptr || e->tag() != CpTag::UTF8) return nullptr;
    return static_cast<const CpUtf8*>(e);
}

const CpNameType* asCpNameType(const CpEntry* e)
{
    if (e == nullptr || e->tag() != CpTag::NAME_TYPE) return nullptr;
    return static_cast<const CpNameType*>(e);
}

const ConstRef* asConstRef(const CpEntry* e)
{
    if (e == nullptr) return nullptr;
    if (e->tag() == CpTag::FIELD_REF || e->tag() == CpTag::METHOD_REF ||
        e->tag() == CpTag::INTERFACE_METHOD_REF) {
        return static_cast<const ConstRef*>(e);
    }
    return nullptr;
}

const ConstDynamic* asConstDynamic(const CpEntry* e)
{
    if (e == nullptr) return nullptr;
    if (e->tag() == CpTag::DYNAMIC || e->tag() == CpTag::INVOKE_DYNAMIC) {
        return static_cast<const ConstDynamic*>(e);
    }
    return nullptr;
}

// ---------- 上游 cffb89ff：ACC_MODULE 降级 ----------

// module-only 结构值必须为 0。带 ACC_MODULE 的类一旦声明了 superclass /
// interface / field / method，说明它在结构上是普通类，应降级处理（清除
// ACC_MODULE 并丢弃模块元数据），对应 JVMS 对 module-info 的约束。
// 原版另有 dropModuleAccess 开关（关闭后改为直接抛 IOException 拒绝读取），
// 因 class_file_reader.hpp 不在本次改动范围，本仓固定为开启（上游默认值）。
inline bool isNonZeroModuleContent(bool moduleAccess, int value)
{
    return moduleAccess && value != 0;
}

// 属性是否携带模块元数据。按属性名判断（与原版 AttributeConstants 匹配一致），
// 覆盖 parse 失败回退为 DefaultAttribute 的场景（此时 kind() 不再是 Module 系）。
bool isModuleAttribute(const Attribute* attribute)
{
    if (attribute == nullptr) return false;
    const std::string& name = attribute->name().text();
    return name == AttributeConstants::MODULE || name == AttributeConstants::MODULE_HASHES ||
           name == AttributeConstants::MODULE_MAIN_CLASS ||
           name == AttributeConstants::MODULE_PACKAGES ||
           name == AttributeConstants::MODULE_RESOLUTION ||
           name == AttributeConstants::MODULE_TARGET;
}

}   // namespace

ClassFile ClassFileReader::read(const std::vector<ca::u8>& code)
{
    try {
        ClassBuilder        builder;
        IndexableByteStream is(code);

        // Magic
        ca::u32 magic = is.readInt();
        if (magic != MAGIC) {
            throwInvalid("Does not start with 0xCAFEBABE");
        }

        // Version
        builder.setVersionMinor(is.readUnsignedShort());
        builder.setVersionMajor(is.readUnsignedShort());

        // Constant pool
        int        numConstants = is.readUnsignedShort();
        int        start        = is.getIndex();
        ConstPool& constPool    = builder.getPool();

        // First pass: populate with placeholders
        for (int i = 1; i < numConstants; i++) {
            auto entry = readPoolEntryBasic(is);
            bool wide  = entry->isWide();
            constPool.add(std::move(entry));
            if (wide) i++;
        }

        // Rewind
        int diff = is.getIndex() - start;
        is.moveBack(diff);

        // Second pass: resolve references
        for (int i = 1; i < numConstants; i++) {
            const CpEntry* entry = constPool.get(i);
            if (entry == nullptr) {
                continue;
            }
            bool wide = entry->isWide();
            readPoolEntryResolve(constPool, entry, is);
            if (wide) i++;
        }

        // Access flags
        builder.setAccess(is.readUnsignedShort());

        // 上游 cffb89ff：先记下原始 flag，降级判定贯穿全部结构检查，
        // 全部检查完才清除 ACC_MODULE。
        bool moduleAccess      = builder.isModule();
        bool dropModuleContent = false;

        // This/super class
        // - module class 不允许有 superclass/fields/methods；任一存在即视为
        //   结构非法的普通类，降级处理。
        int thisIndex  = is.readUnsignedShort();
        int superIndex = is.readUnsignedShort();
        dropModuleContent |= isNonZeroModuleContent(moduleAccess, superIndex);
        const CpEntry* thisEntry  = constPool.get(thisIndex);
        const CpEntry* superEntry = constPool.get(superIndex);
        builder.setThisClass(asCpClass(thisEntry));
        builder.setSuperClass(asCpClass(superEntry));

        // Interfaces
        int numInterfaces = is.readUnsignedShort();
        dropModuleContent |= isNonZeroModuleContent(moduleAccess, numInterfaces);
        for (int i = 0; i < numInterfaces; i++) {
            const CpEntry* e     = constPool.get(is.readUnsignedShort());
            const CpClass* iface = asCpClass(e);
            if (iface == nullptr) {
                throwInvalid("Interface entry was not CpClass");
            }
            builder.addInterface(iface);
        }

        // Fields
        int numFields = is.readUnsignedShort();
        dropModuleContent |= isNonZeroModuleContent(moduleAccess, numFields);
        for (int i = 0; i < numFields; i++) {
            builder.addField(readField(builder, is));
        }

        // Methods
        int numMethods = is.readUnsignedShort();
        dropModuleContent |= isNonZeroModuleContent(moduleAccess, numMethods);
        for (int i = 0; i < numMethods; i++) {
            builder.addMethod(readMethod(builder, is));
        }

        // 结构上是普通类的 module class：不得保留 module 身份与元数据。
        if (dropModuleContent) {
            builder.setAccess(builder.getAccess() & ~modifier_flag::ACC_MODULE);
        }

        // Class attributes（单例属性去重，上游 baad3a72）
        int numAttributes = is.readUnsignedShort();
        std::vector<const Attribute*> attributes = AttributeReader::readAttributes(
            *this, builder, is, numAttributes, AttributeContext(AttributeHolderType::CLASS, 0));
        for (const Attribute* attr : attributes) {
            builder.addAttribute(attr);
        }

        // 模块元数据属性全部丢弃后，CpModule/CpPackage 不再可被引用，一并清理。
        if (dropModuleContent) {
            ConstPool& pool = builder.getPool();
            for (int i = static_cast<int>(pool.size()) - 1; i >= 1; --i) {
                const CpEntry* entry = pool.get(i);
                if (entry == nullptr) continue;
                if (entry->tag() == CpTag::MODULE || entry->tag() == CpTag::PACKAGE) {
                    (void)pool.remove(i);
                }
            }
        }

        auto& poolEntries = builder.getPool();
        // Code 指令持有的 CpRef 操作数是裸指针：remove 即销毁对象，会使这些
        // 指针悬垂（上游 Java 靠 GC 天然存活，transformer 之后才按校验结果
        // stub 掉引用非法常量的指令）。这里先收集全部指令引用，删除时跳过。
        std::vector<const CpEntry*> instructionRefs;
        for (const Method* m : builder.getMethods()) {
            for (const Attribute* a : m->attributes()) {
                if (a->name().text() != AttributeConstants::CODE) continue;
                const auto* code = static_cast<const CodeAttribute*>(a);
                for (const Instruction* insn : code->instructions()) {
                    if (insn->kind() != InsnKind::CpRef) continue;
                    if (const CpEntry* ref =
                            static_cast<const CpRefInstruction*>(insn)->getEntry())
                        instructionRefs.push_back(ref);
                }
            }
        }
        for (int i = static_cast<int>(poolEntries.size()) - 1; i >= 1; --i) {
            const CpEntry* entry = poolEntries.get(i);
            if (entry == nullptr || !Placeholders::containsPlaceholder(entry)) continue;
            // 类头三组引用（thisClass/superClass/interfaces）不经过属性粒度的
            // placeholder 丢弃检查（readAttribute 的 cpAccesses 检查只覆盖
            // attribute 引用）。它们仍指向待销毁条目时保留条目本身，只清理
            // 无引用者；否则 ClassFile 头部持有悬垂指针，write() 解引用已释放
            // 对象产生垃圾字节（issue #876 写回损坏的根因）。
            const CpClass* entryAsClass = asCpClass(entry);
            const auto&    interfaces   = builder.getInterfaces();
            bool headerReferenced = builder.getThisClass() == entryAsClass ||
                                    builder.getSuperClass() == entryAsClass ||
                                    std::find(interfaces.begin(), interfaces.end(), entryAsClass) !=
                                        interfaces.end();
            // 同理，仍被 Code 指令引用的条目保留，避免指令悬垂指针。
            bool codeReferenced =
                std::find(instructionRefs.begin(), instructionRefs.end(), entry) !=
                instructionRefs.end();
            if (!headerReferenced && !codeReferenced) {
                (void)poolEntries.remove(i);
            }
        }

        return builder.build();
    }
    catch (const InvalidClassException&) {
        throw;
    }
    catch (const std::exception& ex) {
        throw InvalidClassException(ex);
    }
}

// ====================== pool 2-pass ======================

std::unique_ptr<CpEntry> ClassFileReader::readPoolEntryBasic(IndexableByteStream& is)
{
    int tag = is.readUnsignedByte();
    switch (static_cast<CpTag>(tag)) {
    case CpTag::UTF8:
    {
        return std::make_unique<CpUtf8>(readMutf8AsUtf8(is, lenientTextDecoder()));
    }
    case CpTag::INTEGER:
    {
        return std::make_unique<CpInt>(static_cast<ca::i32>(is.readInt()));
    }
    case CpTag::FLOAT:
    {
        return std::make_unique<CpFloat>(is.readFloat());
    }
    case CpTag::LONG:
    {
        return std::make_unique<CpLong>(static_cast<ca::i64>(is.readLong()));
    }
    case CpTag::DOUBLE:
    {
        return std::make_unique<CpDouble>(is.readDouble());
    }
    case CpTag::STRING:
    {
        is.readUnsignedShort();
        return std::make_unique<CpString>(Placeholders::utf8());
    }
    case CpTag::CLASS:
    {
        is.readUnsignedShort();
        return std::make_unique<CpClass>(Placeholders::utf8());
    }
    case CpTag::FIELD_REF:
    {
        is.readUnsignedShort();
        is.readUnsignedShort();
        return std::make_unique<CpFieldRef>(Placeholders::klass(), Placeholders::nameType());
    }
    case CpTag::METHOD_REF:
    {
        is.readUnsignedShort();
        is.readUnsignedShort();
        return std::make_unique<CpMethodRef>(Placeholders::klass(), Placeholders::nameType());
    }
    case CpTag::INTERFACE_METHOD_REF:
    {
        is.readUnsignedShort();
        is.readUnsignedShort();
        return std::make_unique<CpInterfaceMethodRef>(Placeholders::klass(),
                                                      Placeholders::nameType());
    }
    case CpTag::NAME_TYPE:
    {
        is.readUnsignedShort();
        is.readUnsignedShort();
        return std::make_unique<CpNameType>(Placeholders::utf8(), Placeholders::utf8());
    }
    case CpTag::DYNAMIC:
    {
        ca::u16 bsmIndex = is.readUnsignedShort();
        is.readUnsignedShort();
        return std::make_unique<CpDynamic>(bsmIndex, Placeholders::nameType());
    }
    case CpTag::METHOD_HANDLE:
    {
        ca::i8 kind = is.readByte();
        is.readUnsignedShort();
        return std::make_unique<CpMethodHandle>(static_cast<ca::u8>(kind),
                                                Placeholders::constRef());
    }
    case CpTag::METHOD_TYPE:
    {
        is.readUnsignedShort();
        return std::make_unique<CpMethodType>(Placeholders::utf8());
    }
    case CpTag::INVOKE_DYNAMIC:
    {
        ca::u16 bsmIndex = is.readUnsignedShort();
        is.readUnsignedShort();
        return std::make_unique<CpInvokeDynamic>(bsmIndex, Placeholders::nameType());
    }
    case CpTag::MODULE:
    {
        is.readUnsignedShort();
        return std::make_unique<CpModule>(Placeholders::utf8());
    }
    case CpTag::PACKAGE:
    {
        is.readUnsignedShort();
        return std::make_unique<CpPackage>(Placeholders::utf8());
    }
    default:
        if (skipUnknownPoolTags_) {
            // 未知 tag：非 JVM 合法 tag，载荷宽度无规范可依；本仓容错约定按
            // 最小 1 字节跳过（与 crasher 语料的保留 tag 布局一致），返回含
            // placeholder 的条目，cleanup 会移除这些条目。
            is.skipBytes(1);
            return std::make_unique<CpModule>(Placeholders::utf8());
        }
        throwInvalid("Unknown constant-pool tag");
    }
}

void ClassFileReader::readPoolEntryResolve(ConstPool& pool, const CpEntry* entry,
                                           IndexableByteStream& is)
{
    if (entry == nullptr) return;

    int tag = is.readUnsignedByte();
    if (static_cast<CpTag>(tag) != entry->tag()) {
        throwInvalid("Constant pool tag mismatch");
    }

    switch (static_cast<CpTag>(tag)) {
    case CpTag::UTF8:
    {
        (void)readMutf8AsUtf8(is, lenientTextDecoder());
        break;
    }
    case CpTag::INTEGER:
    {
        is.readInt();
        break;
    }
    case CpTag::FLOAT:
    {
        is.readFloat();
        break;
    }
    case CpTag::LONG:
    {
        is.readLong();
        break;
    }
    case CpTag::DOUBLE:
    {
        is.readDouble();
        break;
    }
    case CpTag::STRING:
    {
        const CpEntry* param = pool.get(is.readUnsignedShort());
        const CpUtf8*  utf8  = asCpUtf8(param);
        if (utf8 != nullptr) {
            const_cast<CpString*>(static_cast<const CpString*>(entry))->setValue(utf8);
        }
        break;
    }
    case CpTag::CLASS:
    {
        const CpEntry* param = pool.get(is.readUnsignedShort());
        const CpUtf8*  utf8  = asCpUtf8(param);
        if (utf8 != nullptr) {
            const_cast<CpClass*>(static_cast<const CpClass*>(entry))->setName(utf8);
        }
        break;
    }
    case CpTag::FIELD_REF:
    case CpTag::METHOD_REF:
    case CpTag::INTERFACE_METHOD_REF:
    {
        const CpEntry*    p1    = pool.get(is.readUnsignedShort());
        const CpEntry*    p2    = pool.get(is.readUnsignedShort());
        const CpClass*    clazz = asCpClass(p1);
        const CpNameType* nt    = asCpNameType(p2);
        const ConstRef*   ref   = asConstRef(entry);
        if (ref != nullptr && clazz != nullptr && nt != nullptr) {
            const_cast<ConstRef*>(ref)->setClassRef(clazz);
            const_cast<ConstRef*>(ref)->setNameType(nt);
        }
        break;
    }
    case CpTag::NAME_TYPE:
    {
        const CpEntry*    p1   = pool.get(is.readUnsignedShort());
        const CpEntry*    p2   = pool.get(is.readUnsignedShort());
        const CpUtf8*     name = asCpUtf8(p1);
        const CpUtf8*     type = asCpUtf8(p2);
        const CpNameType* nt   = asCpNameType(entry);
        if (nt != nullptr && name != nullptr && type != nullptr) {
            const_cast<CpNameType*>(nt)->setName(name);
            const_cast<CpNameType*>(nt)->setType(type);
        }
        break;
    }
    case CpTag::DYNAMIC:
    case CpTag::INVOKE_DYNAMIC:
    {
        ca::u16             bsmIndex = is.readUnsignedShort();
        const CpEntry*      param    = pool.get(is.readUnsignedShort());
        const CpNameType*   nt       = asCpNameType(param);
        const ConstDynamic* dyn      = asConstDynamic(entry);
        if (dyn != nullptr) {
            const_cast<ConstDynamic*>(dyn)->setBsmIndex(bsmIndex);
            if (nt != nullptr) {
                const_cast<ConstDynamic*>(dyn)->setNameType(nt);
            }
        }
        break;
    }
    case CpTag::METHOD_HANDLE:
    {
        ca::i8                kind  = is.readByte();
        const CpEntry*        param = pool.get(is.readUnsignedShort());
        const ConstRef*       ref   = asConstRef(param);
        const CpMethodHandle* mh    = static_cast<const CpMethodHandle*>(entry);
        const_cast<CpMethodHandle*>(mh)->setKind(static_cast<ca::u8>(kind));
        if (ref != nullptr) {
            const_cast<CpMethodHandle*>(mh)->setReference(ref);
        }
        break;
    }
    case CpTag::METHOD_TYPE:
    {
        const CpEntry*      param = pool.get(is.readUnsignedShort());
        const CpUtf8*       utf8  = asCpUtf8(param);
        const CpMethodType* mt    = static_cast<const CpMethodType*>(entry);
        if (utf8 != nullptr) {
            const_cast<CpMethodType*>(mt)->setDescriptor(utf8);
        }
        break;
    }
    case CpTag::MODULE:
    {
        const CpEntry*  param = pool.get(is.readUnsignedShort());
        const CpUtf8*   utf8  = asCpUtf8(param);
        const CpModule* mod   = static_cast<const CpModule*>(entry);
        if (utf8 != nullptr) {
            const_cast<CpModule*>(mod)->setName(utf8);
        }
        break;
    }
    case CpTag::PACKAGE:
    {
        const CpEntry*   param = pool.get(is.readUnsignedShort());
        const CpUtf8*    utf8  = asCpUtf8(param);
        const CpPackage* pkg   = static_cast<const CpPackage*>(entry);
        if (utf8 != nullptr) {
            const_cast<CpPackage*>(pkg)->setPackageName(utf8);
        }
        break;
    }
    default:
        if (skipUnknownPoolTags_) {
            // 第二遍：跳过未知 tag 的数据字节，不解析
            is.skipBytes(1);
            break;
        }
        throw InvalidClassException(std::string("Unknown CP tag during resolve: ") +
                                    std::to_string(tag));
    }
}

// ====================== field/method ======================

const Field* ClassFileReader::readField(ClassBuilder& builder, IndexableByteStream& is)
{
    int access        = is.readUnsignedShort();
    int nameIndex     = is.readUnsignedShort();
    int descIndex     = is.readUnsignedShort();
    int numAttributes = is.readUnsignedShort();

    // 单例属性去重（上游 baad3a72）。
    std::vector<const Attribute*> attributes = AttributeReader::readAttributes(
        *this, builder, is, numAttributes, AttributeContext(AttributeHolderType::FIELD, access));

    const CpEntry* nameEntry = builder.getPool().get(nameIndex);
    const CpEntry* descEntry = builder.getPool().get(descIndex);
    const CpUtf8*  name      = asCpUtf8(nameEntry);
    const CpUtf8*  type      = asCpUtf8(descEntry);
    if (name == nullptr || type == nullptr) {
        throwInvalid("Field name/type index do not point to UTF8 values");
    }
    return builder.createField(std::move(attributes), access, name, type);
}

const Method* ClassFileReader::readMethod(ClassBuilder& builder, IndexableByteStream& is)
{
    int access        = is.readUnsignedShort();
    int nameIndex     = is.readUnsignedShort();
    int descIndex     = is.readUnsignedShort();
    int numAttributes = is.readUnsignedShort();

    // 单例属性去重（上游 baad3a72）。
    std::vector<const Attribute*> attributes = AttributeReader::readAttributes(
        *this, builder, is, numAttributes, AttributeContext(AttributeHolderType::METHOD, access));

    const CpEntry* nameEntry = builder.getPool().get(nameIndex);
    const CpEntry* descEntry = builder.getPool().get(descIndex);
    const CpUtf8*  name      = asCpUtf8(nameEntry);
    const CpUtf8*  type      = asCpUtf8(descEntry);
    if (name == nullptr || type == nullptr) {
        throwInvalid("Method name/type index do not point to UTF8 values: name=" +
                     std::to_string(nameIndex) +
                     " tag=" + std::to_string(nameEntry ? static_cast<int>(nameEntry->tag()) : -1) +
                     ", desc=" + std::to_string(descIndex) +
                     " tag=" + std::to_string(descEntry ? static_cast<int>(descEntry->tag()) : -1));
    }
    return builder.createMethod(std::move(attributes), access, name, type);
}

}   // namespace jclassfile::io
