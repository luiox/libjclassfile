// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/EnumElementValue.java
// 状态：阶段 6 完整实现

#include "jclassfile/model/annotation/enum_element_value.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> EnumElementValue::cpAccesses() const
{
    return {name_, type_};
}

}   // namespace jclassfile::model
