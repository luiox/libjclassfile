// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/PrimitiveElementValue.java
// 状态：阶段 6 完整实现

#include "jclassfile/model/annotation/primitive_element_value.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> PrimitiveElementValue::cpAccesses() const
{
    return {value_};
}

}   // namespace jclassfile::model
