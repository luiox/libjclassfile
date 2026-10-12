// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/AnnotationElementValue.java
// 状态：阶段 6 完整实现

#include "jclassfile/model/annotation/annotation_element_value.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> AnnotationElementValue::cpAccesses() const
{
    if (annotation_ == nullptr) return {};
    return annotation_->cpAccesses();
}

int AnnotationElementValue::computeLength() const noexcept
{
    return 1 + (annotation_ ? annotation_->computeLength() : 0);
}

}   // namespace jclassfile::model
