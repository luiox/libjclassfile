#pragma once

#include <memory>
#include <vector>

#include "jclassfile/io/attribute_writer.hpp"
#include "jclassfile/io/fallback_instruction_writer.hpp"
#include "jclassfile/invalid_class_exception.hpp"
#include "libca/core/datatype.hpp"
#include "jclassfile/model/class_file.hpp"

namespace jclassfile::io {

class ClassFileWriter
{
public:
    ClassFileWriter() = default;

    [[nodiscard]] std::vector<ca::u8> write(model::ClassFile& clazz);

protected:
    [[nodiscard]] const FallbackInstructionWriter* getFallbackInstructionWriter() const noexcept
    {
        return fallbackInstructionWriter_.get();
    }

    [[nodiscard]] virtual std::unique_ptr<FallbackInstructionWriter> newFallbackInstructionWriter(
        model::ClassFile& /*clazz*/)
    {
        return FallbackInstructionWriter::fail();
    }

private:
    void writeCpEntry(util::GrowingByteBuffer& out, const model::CpEntry* entry);
    void writeField(util::GrowingByteBuffer& out, const model::Field* field);
    void writeMethod(util::GrowingByteBuffer& out, const model::Method* method);
    void writeAttribute(util::GrowingByteBuffer& out, const model::Attribute* attribute);

    std::unique_ptr<FallbackInstructionWriter> fallbackInstructionWriter_;
    std::unique_ptr<AttributeWriter>           attributeWriter_;
};

}   // namespace jclassfile::io
