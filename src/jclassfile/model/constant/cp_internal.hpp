// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpInternal.java
// 状态：阶段 2 实现
// 备注：原版是 marker interface，标记 internal-only entry（ConstRefInternal / ConstDynamic）。
//       C++ 没有 interface 概念，注释说明 "marker equivalent"。

#pragma once

namespace jclassfile::model {

class CpInternal
{
public:
    virtual ~CpInternal() = default;
};

}   // namespace jclassfile::model
