// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/CodeAttribute.java

#include "jclassfile/model/attribute/code_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> CodeAttribute::ExceptionTableEntry::cpAccesses() const
{
    if (catchType_ != nullptr) return {catchType_};
    return {};
}

std::vector<const CpEntry*> CodeAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const Attribute* a : attributes_) {
        for (const CpEntry* e : a->cpAccesses()) set.push_back(e);
    }
    for (const ExceptionTableEntry& e : exceptionTable_) {
        for (const CpEntry* ce : e.cpAccesses()) set.push_back(ce);
    }
    // 阶段 3 Instruction stub 不实现 CpAccessor；阶段 5 真正实现 Instruction/CpRefInstruction
    // 时在此处调用 insn->cpAccesses()。
    return set;
}

int CodeAttribute::computeInternalLength() const noexcept
{
    // u2: max_stack
    // u2: max_locals
    int len = 4;
    // u4: code_length
    // u1 * X: CODE
    len += 4;
    int insnSize = 0;
    for (const Instruction* insn : instructions_) insnSize += insn->computeSize();
    len += insnSize;
    // u2: exception_table_length
    // u2 * 4 * X: EXCEPTIONS
    len += 2;
    len += 8 * static_cast<int>(exceptionTable_.size());
    // u2: attributes_count
    // ??: ATTRIBS
    len += 2;
    for (const Attribute* a : attributes_) len += a->computeCompleteLength();
    return len;
}

}   // namespace jclassfile::model
