// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/Utf8ElementValue.java
// 状态：阶段 6 完整实现
// 备注：cpAccesses 返回单元素 {value_}（与原版 Collections.singleton 一致）。

#include "jclassfile/model/annotation/utf8_element_value.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> Utf8ElementValue::cpAccesses() const
{
    return {value_};
}

}   // namespace jclassfile::model
