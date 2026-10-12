// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/Annotation.java
// 状态：阶段 6 完整实现
// 备注：cpAccesses 收集 {type_} + 各 ElementValue 的 cpAccesses。
//       computeLength = 4 (u2 type_index + u2 num_pairs) + sum(2 + value.computeLength)。

#include "jclassfile/model/annotation/annotation.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> Annotation::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    set.push_back(type_);
    for (const auto& [_, value] : values_) {
        if (value == nullptr) continue;
        for (const CpEntry* e : value->cpAccesses()) set.push_back(e);
    }
    return set;
}

int Annotation::computeLength() const noexcept
{
    int length = 4;   // u2 type_index + u2 num_element_value_pairs
    for (const auto& [_, value] : values_) {
        if (value == nullptr) continue;
        // u2 name_index
        // ?? value
        length += 2;
        length += value->computeLength();
    }
    return length;
}

}   // namespace jclassfile::model
