// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/BasicInstruction.java
// 状态：阶段 5b 实现
// 备注：无操作数指令（NOP / ICONST_* / ALOAD_0 / IADD / RETURN 等）的最小包装。
//       computeSize = 1（仅 opcode 字节）。

#pragma once

#include "jclassfile/model/instruction/instruction.hpp"

namespace jclassfile::model {

class BasicInstruction final : public Instruction
{
public:
    explicit BasicInstruction(int opcode) noexcept
        : Instruction(opcode)
    {}

    [[nodiscard]] InsnKind kind() const noexcept override { return InsnKind::Basic; }
    [[nodiscard]] int      computeSize() const noexcept override { return 1; }
};

}   // namespace jclassfile::model
