// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/IntOperandInstruction.java
// 状态：阶段 5b 实现
// 备注：单 int 操作数指令的通用包装。
//       computeSize 按 opcode 分支（与原版 switch 完全一致）：
//         * 1 字节操作数：BIPUSH / ILOAD-LSTORE / ALOAD-ASTORE / DLOAD-DSTORE / FLOAD-FSTORE / RET
//         / NEWARRAY
//         * 2 字节操作数：SIPUSH / IF*-IF* / GOTO / JSR / IFNULL / IFNONNULL
//         * 4 字节操作数：GOTO_W / JSR_W
//       注：原版 Java `int` 与字节序无关；C++ 写入时通过 putShort / putInt big-endian，
//       读取时通过 readShort / readInt 自动还原。

#pragma once

#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"

namespace jclassfile::model {

class IntOperandInstruction final : public Instruction
{
public:
    IntOperandInstruction(int opcode, int operand) noexcept
        : Instruction(opcode)
        , operand_(operand)
    {}

    [[nodiscard]] InsnKind kind() const noexcept override { return InsnKind::IntOperand; }

    [[nodiscard]] int getOperand() const noexcept { return operand_; }
    void              setOperand(int operand) noexcept { operand_ = operand; }

    [[nodiscard]] int computeSize() const noexcept override;

    [[nodiscard]] bool equals(const Instruction& other) const noexcept override
    {
        if (other.kind() != InsnKind::IntOperand) return false;
        if (getOpcode() != other.getOpcode()) return false;
        const auto& that = static_cast<const IntOperandInstruction&>(other);
        return operand_ == that.operand_;
    }

    [[nodiscard]] std::string toString() const override
    {
        return Instruction::toString() + " operand=" + std::to_string(operand_);
    }

private:
    int operand_;
};

}   // namespace jclassfile::model
