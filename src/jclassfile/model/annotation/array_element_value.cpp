// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/ArrayElementValue.java
// 状态：阶段 6 完整实现
// 备注：cpAccesses / computeLength 与原版 lambda / sum 一致。

#include "jclassfile/model/annotation/array_element_value.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ArrayElementValue::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    for (const auto& value : array_) {
        if (value == nullptr) continue;
        for (const CpEntry* e : value->cpAccesses()) set.push_back(e);
    }
    return set;
}

int ArrayElementValue::computeLength() const noexcept
{
    int len = 3;   // u1 tag + u2 num_elements
    for (const auto& value : array_) {
        if (value == nullptr) continue;
        len += value->computeLength();
    }
    return len;
}

}   // namespace jclassfile::model
