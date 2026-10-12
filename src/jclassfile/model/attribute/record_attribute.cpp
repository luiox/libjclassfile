// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/RecordAttribute.java

#include "jclassfile/model/attribute/record_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> RecordAttribute::RecordComponent::cpAccesses() const
{
    return {name_, desc_};
}

int RecordAttribute::RecordComponent::length() const noexcept
{
    // u2: name_index
    // u2: desc_index
    // u2: attribute_count
    // ??  attributes[]
    int len = 6;
    for (const Attribute* a : attributes_) len += a->computeCompleteLength();
    return len;
}

std::vector<const CpEntry*> RecordAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const RecordComponent& c : components_) {
        for (const CpEntry* e : c.cpAccesses()) set.push_back(e);
    }
    return set;
}

int RecordAttribute::computeInternalLength() const noexcept
{
    int len = 2;
    for (const RecordComponent& c : components_) len += c.length();
    return len;
}

}   // namespace jclassfile::model
