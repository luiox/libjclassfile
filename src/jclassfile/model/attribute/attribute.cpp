// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/Attribute.java
// 状态：阶段 3 实现
// 备注：原版 cpAccesses() 返回 Set<CpEntry>；C++ 沿用阶段 2 约定的 std::vector<const CpEntry*>。

#include "jclassfile/model/attribute/attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> Attribute::cpAccesses() const
{
    return {name_};
}

}   // namespace jclassfile::model
