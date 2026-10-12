// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/CpRefInstruction.java
// 状态：阶段 5b 实现
// 备注：引用常量池的指令（LDC / LDC_W / LDC2_W / GETSTATIC / PUTSTATIC / GETFIELD /
//       PUTFIELD / INVOKE* / NEW / ANEWARRAY / CHECKCAST / INSTANCEOF）。
//       computeSize：LDC=2，INVOKEDYNAMIC/INVOKEINTERFACE=5（带 2 字节 padding），其他=3。
//       cpAccesses() 返回 {entry}（与 Java Set<CpEntry> 行为一致，转为 vector）。
//       equals() 通过 kind() + opcode + entry_ 三重比较实现；C++ 无 RTTI 不能用 dynamic_cast。

#pragma once

#include "jclassfile/model/instruction/instruction.hpp"

namespace jclassfile::model {

class CpEntry;

class CpRefInstruction final : public Instruction
{
public:
    CpRefInstruction(int opcode, const CpEntry* entry) noexcept
        : Instruction(opcode)
        , entry_(entry)
    {}

    [[nodiscard]] InsnKind kind() const noexcept override { return InsnKind::CpRef; }

    [[nodiscard]] const CpEntry* getEntry() const noexcept { return entry_; }
    void                         setEntry(const CpEntry* entry) noexcept { entry_ = entry; }

    [[nodiscard]] int computeSize() const noexcept override;

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override
    {
        return entry_ ? std::vector<const CpEntry*>{entry_} : std::vector<const CpEntry*>{};
    }

    [[nodiscard]] bool equals(const Instruction& other) const noexcept override;

    [[nodiscard]] std::string toString() const override;

private:
    const CpEntry* entry_;
};

}   // namespace jclassfile::model
