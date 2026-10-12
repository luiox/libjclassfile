// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/WideInstruction.java
// 状态：阶段 5b 实现
// 备注：wide 指令包装另一个 Instruction。computeSize 按 backing opcode 分支：
//         * IINC → 6 字节（opcode wide + opcode iinc + 2 字节 var + 2 字节 const）
//         * 其他 → 4 字节（opcode wide + backing opcode + 2 字节 var/operand）
//       backing 用 std::unique_ptr 持有所有权（Java 隐式引用，CAFED00D 中 backing 是 final 字段，
//       不修改；C++ 保持 unique_ptr 表达"独占所有权"）。

#pragma once

#include <memory>
#include <string>

#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"

namespace jclassfile::model {

class WideInstruction final : public Instruction
{
public:
    explicit WideInstruction(std::unique_ptr<Instruction> backing) noexcept
        : Instruction(Opcodes::WIDE)
        , backing_(std::move(backing))
    {}

    [[nodiscard]] InsnKind kind() const noexcept override { return InsnKind::Wide; }

    [[nodiscard]] const Instruction* getBacking() const noexcept { return backing_.get(); }
    [[nodiscard]] Instruction*       getBacking() noexcept { return backing_.get(); }

    [[nodiscard]] int computeSize() const noexcept override
    {
        // IINC backing 时 6 字节；其他 4 字节
        if (backing_ && backing_->getOpcode() == Opcodes::IINC) {
            return 6;
        }
        return 4;
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override
    {
        return backing_ ? backing_->cpAccesses() : std::vector<const CpEntry*>{};
    }

    [[nodiscard]] bool equals(const Instruction& other) const noexcept override
    {
        if (other.kind() != InsnKind::Wide) return false;
        const auto& that = static_cast<const WideInstruction&>(other);
        if (backing_.get() == that.backing_.get()) return true;
        if (backing_ == nullptr || that.backing_ == nullptr) return false;
        return backing_->equals(*that.backing_);
    }

    [[nodiscard]] std::string toString() const override
    {
        std::string s = Instruction::toString() + "backing=";
        if (backing_)
            s += backing_->toString();
        else
            s += "null";
        return s;
    }

private:
    std::unique_ptr<Instruction> backing_;
};

}   // namespace jclassfile::model
