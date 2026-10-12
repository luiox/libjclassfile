// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/CodeUtilities.java
// 状态：阶段 3 实现
// 备注：原版 interface 含 getInstructions() 抽象方法 + 4 个 default 方法 + 2 个 static 方法。
//       C++ 用 abstract class 表达；default 方法写成 inline。Instruction 是阶段 5 stub，
//       此处只需 forward decl + 在 .cpp 引用 methods 时完整包含。
//       CodeUtilities::wrap 在原版返回 () -> instructions lambda；C++ 用私有 Wrapper 类实现。

#pragma once

#include <memory>
#include <vector>

#include "jclassfile/model/instruction/instruction.hpp"

namespace jclassfile::model {

class Instruction;

class CodeUtilities
{
public:
    virtual ~CodeUtilities() = default;

    [[nodiscard]] virtual const std::vector<const Instruction*>& instructions() const = 0;

    [[nodiscard]] int indexOf(const Instruction* instruction) const noexcept
    {
        const auto& insns = instructions();
        for (std::size_t i = 0; i < insns.size(); i++) {
            if (insns[i] == instruction) return static_cast<int>(i);
        }
        return -1;
    }

    [[nodiscard]] int computeOffsetOf(const Instruction* instruction) const noexcept
    {
        int idx = indexOf(instruction);
        if (idx == 0) return 0;
        if (idx < 0) return -1;
        const auto& insns  = instructions();
        int         offset = 0;
        for (int i = 0; i < idx; i++) {
            offset += insns[i]->computeSize();
        }
        return offset;
    }

    [[nodiscard]] const Instruction* getInstructionAtOffset(int offset) const noexcept
    {
        const auto& insns   = instructions();
        int         current = 0;
        for (const Instruction* insn : insns) {
            if (current == offset)
                return insn;
            else if (current > offset)
                break;
            current += insn->computeSize();
        }
        return nullptr;
    }

    [[nodiscard]] const Instruction* getContainingInstructionAtOffset(int offset) const noexcept
    {
        const auto& insns   = instructions();
        int         current = 0;
        for (const Instruction* insn : insns) {
            int size = insn->computeSize();
            if (offset >= current && offset < current + size)
                return insn;
            else if (current > offset)
                break;
            current += size;
        }
        return nullptr;
    }

    [[nodiscard]] int computeSize() const noexcept
    {
        const auto&        insns = instructions();
        const Instruction* last  = insns.back();
        return computeOffsetOf(last) + last->computeSize();
    }

    [[nodiscard]] static bool isBranch(const Instruction* instruction) noexcept;

    [[nodiscard]] static bool isTerminalOrAlwaysTakeFlowControl(
        const Instruction* instruction) noexcept;

    [[nodiscard]] static std::unique_ptr<CodeUtilities> wrap(
        const std::vector<const Instruction*>& instructions);

protected:
    CodeUtilities() = default;
};

}   // namespace jclassfile::model
