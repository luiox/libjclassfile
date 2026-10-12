// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/MultiANewArrayInstruction.java
// 状态：阶段 5b 实现
// 备注：multianewarray 指令（CpClass* descriptor + 维度数）。implements CpAccessor（cpAccesses 返回
// {descriptor}）。
//       computeSize = 4（opcode + 2 字节 descriptor index + 1 字节 dimensions）。

#pragma once

#include <string>
#include <vector>

#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"

namespace jclassfile::model {

class CpClass;

class MultiANewArrayInstruction final : public Instruction
{
public:
    MultiANewArrayInstruction(const CpClass* descriptor, int dimensions) noexcept
        : Instruction(Opcodes::MULTIANEWARRAY)
        , descriptor_(descriptor)
        , dimensions_(dimensions)
    {}

    [[nodiscard]] InsnKind kind() const noexcept override { return InsnKind::MultiANewArray; }

    [[nodiscard]] int getDimensions() const noexcept { return dimensions_; }
    void              setDimensions(int dimensions) noexcept { dimensions_ = dimensions; }

    [[nodiscard]] const CpClass* getDescriptor() const noexcept { return descriptor_; }
    void setDescriptor(const CpClass* descriptor) noexcept { descriptor_ = descriptor; }

    [[nodiscard]] int computeSize() const noexcept override { return 4; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override
    {
        return descriptor_ ? std::vector<const CpEntry*>{descriptor_}
                           : std::vector<const CpEntry*>{};
    }

    [[nodiscard]] bool equals(const Instruction& other) const noexcept override
    {
        if (other.kind() != InsnKind::MultiANewArray) return false;
        const auto& that = static_cast<const MultiANewArrayInstruction&>(other);
        return dimensions_ == that.dimensions_ && descriptor_ == that.descriptor_;
    }

    [[nodiscard]] std::string toString() const override
    {
        return Instruction::toString() + " dimensions=" + std::to_string(dimensions_) +
               ", descriptor=" +
               (descriptor_ ? std::to_string(descriptor_->index()) : std::string("null"));
    }

private:
    const CpClass* descriptor_;
    int            dimensions_;
};

}   // namespace jclassfile::model
