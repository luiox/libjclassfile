// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/CpRefInstruction.java
// 状态：阶段 5b 实现

#include "jclassfile/model/instruction/cp_ref_instruction.hpp"

#include "jclassfile/model/instruction/opcodes.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

int CpRefInstruction::computeSize() const noexcept
{
    using namespace Opcodes;
    int op = getOpcode();
    if (op == LDC) {
        return 1 + 1;   // 1 字节 opcode + 1 字节 index
    }
    if (op == INVOKEDYNAMIC || op == INVOKEINTERFACE) {
        return 1 + 2 + 2;   // opcode + 2 字节 index + 2 字节 padding
    }
    return 1 + 2;   // opcode + 2 字节 index
}

bool CpRefInstruction::equals(const Instruction& other) const noexcept
{
    if (other.kind() != InsnKind::CpRef) return false;
    if (getOpcode() != other.getOpcode()) return false;
    // kind 相同 + opcode 相同 ⇒ 类型必为 CpRefInstruction（每 opcode 唯一映射）
    const auto& that = static_cast<const CpRefInstruction&>(other);
    return entry_ == that.entry_;
}

std::string CpRefInstruction::toString() const
{
    std::string base = Instruction::toString();
    base += " entry=";
    if (entry_ != nullptr) {
        base += std::to_string(entry_->index());
    }
    else {
        base += "null";
    }
    return base;
}

}   // namespace jclassfile::model
