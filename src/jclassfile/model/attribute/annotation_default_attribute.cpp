// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/AnnotationDefaultAttribute.java

#include "jclassfile/model/attribute/annotation_default_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> AnnotationDefaultAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const CpEntry* e : elementValue_->cpAccesses()) set.push_back(e);
    return set;
}

int AnnotationDefaultAttribute::computeInternalLength() const noexcept
{
    return elementValue_->computeLength();
}

}   // namespace jclassfile::model
