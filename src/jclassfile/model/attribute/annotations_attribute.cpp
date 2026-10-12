// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/AnnotationsAttribute.java

#include "jclassfile/model/attribute/annotations_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> AnnotationsAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const auto& a : annotations_) {
        for (const CpEntry* e : a->cpAccesses()) set.push_back(e);
    }
    return set;
}

int AnnotationsAttribute::computeInternalLength() const noexcept
{
    int len = 2;
    for (const auto& a : annotations_) len += a->computeLength();
    return len;
}

}   // namespace jclassfile::model
