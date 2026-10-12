// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/IincInstruction.java
// 状态：阶段 5b 实现
// 备注：iinc 指令固定 opcode=IINC（132），含 varIndex + constVal。computeSize=3（普通）或
// 6（WIDE）。
//       var 索引在普通形式为 1 字节无符号（& 0xFF），constVal 为 1 字节有符号。

#pragma once

#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"

namespace jclassfile::model {

class IincInstruction final : public Instruction
{
public:
    IincInstruction(int var, int increment) noexcept
        : Instruction(Opcodes::IINC)
        , var_(var)
        , increment_(increment)
    {}

    [[nodiscard]] InsnKind kind() const noexcept override { return InsnKind::Iinc; }

    [[nodiscard]] int getVar() const noexcept { return var_; }
    void              setVar(int var) noexcept { var_ = var; }

    [[nodiscard]] int getIncrement() const noexcept { return increment_; }
    void              setIncrement(int increment) noexcept { increment_ = increment; }

    [[nodiscard]] int computeSize() const noexcept override { return 3; }

    [[nodiscard]] bool equals(const Instruction& other) const noexcept override
    {
        if (other.kind() != InsnKind::Iinc) return false;
        const auto& that = static_cast<const IincInstruction&>(other);
        return var_ == that.var_ && increment_ == that.increment_;
    }

    [[nodiscard]] std::string toString() const override
    {
        return Instruction::toString() + " var=" + std::to_string(var_) +
               ", incr=" + std::to_string(increment_);
    }

private:
    int var_;
    int increment_;
};

}   // namespace jclassfile::model
