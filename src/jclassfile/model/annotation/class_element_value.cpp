// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/ClassElementValue.java
// 状态：阶段 6 完整实现

#include "jclassfile/model/annotation/class_element_value.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ClassElementValue::cpAccesses() const
{
    return {classEntry_};
}

}   // namespace jclassfile::model
