// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/TypeAnnotation.java
// 状态：阶段 6 完整实现

#include "jclassfile/model/annotation/type_annotation.hpp"

namespace jclassfile::model {

int TypeAnnotation::computeLength() const noexcept
{
    int length = 1;                                             // u1: target_type
    length += targetInfo_ ? targetInfo_->computeLength() : 0;   // ??: target_info
    length += typePath_ ? typePath_->computeLength() : 0;       // type_path
    return length + Annotation::computeLength();
}

}   // namespace jclassfile::model
