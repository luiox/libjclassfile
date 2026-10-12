// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CrossCpReferencing.java
// 状态：阶段 2 实现
// 备注：原版 interface 含 getReferences() 方法，Placeholders.containsPlaceholder 用它 DFS 引用链。

#pragma once

#include <vector>

namespace jclassfile::model {

class CpEntry;

class CrossCpReferencing
{
public:
    virtual ~CrossCpReferencing()                                           = default;
    [[nodiscard]] virtual std::vector<const CpEntry*> getReferences() const = 0;
};

}   // namespace jclassfile::model
