// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/StackMapTableAttribute.java

#include "jclassfile/model/attribute/stack_map_table_attribute.hpp"

#include <memory>

namespace jclassfile::model {

std::vector<const CpEntry*> StackMapTableAttribute::TypeInfo::cpAccesses() const
{
    return {};
}

std::vector<const CpEntry*> StackMapTableAttribute::ObjectVariableInfo::cpAccesses() const
{
    return {classEntry_};
}

std::vector<const CpEntry*> StackMapTableAttribute::StackMapFrame::cpAccesses() const
{
    return {};
}

std::vector<const CpEntry*> StackMapTableAttribute::SameLocalsOneStackItem::cpAccesses() const
{
    return stack_->cpAccesses();
}

std::vector<const CpEntry*>
StackMapTableAttribute::SameLocalsOneStackItemExtended::cpAccesses() const
{
    return stack_->cpAccesses();
}

int StackMapTableAttribute::AppendFrame::getLength() const noexcept
{
    int length = 1 + 2;   // u1: frame_type, u2: offset_delta
    for (const auto& local : additionalLocals_) length += local->getLength();
    return length;
}

std::vector<const CpEntry*> StackMapTableAttribute::AppendFrame::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    for (const auto& info : additionalLocals_) {
        for (const CpEntry* e : info->cpAccesses()) set.push_back(e);
    }
    return set;
}

int StackMapTableAttribute::FullFrame::getLength() const noexcept
{
    int length = 1 + 2;   // u1: frame_type, u2: offset_delta
    length += 2;          // u2 number_of_locals
    for (const auto& local : locals_) length += local->getLength();
    length += 2;   // u2 number_of_stack_items
    for (const auto& stackType : stack_) length += stackType->getLength();
    return length;
}

std::vector<const CpEntry*> StackMapTableAttribute::FullFrame::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    for (const auto& info : locals_) {
        for (const CpEntry* e : info->cpAccesses()) set.push_back(e);
    }
    for (const auto& info : stack_) {
        for (const CpEntry* e : info->cpAccesses()) set.push_back(e);
    }
    return set;
}

std::vector<const CpEntry*> StackMapTableAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const auto& f : frames_) {
        for (const CpEntry* e : f->cpAccesses()) set.push_back(e);
    }
    return set;
}

int StackMapTableAttribute::computeInternalLength() const noexcept
{
    int len = 2;   // u2: number_of_entries
    for (const auto& f : frames_) len += f->getLength();
    return len;
}

}   // namespace jclassfile::model
