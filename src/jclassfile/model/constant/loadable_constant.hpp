// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/LoadableConstant.java
// 状态：阶段 2 实现
// 备注：marker interface，标记可被 ldc 指令加载的
// entry（Int/Float/Long/Double/String/Class/MethodHandle/MethodType + Dynamic）。

#pragma once

namespace jclassfile::model {

class LoadableConstant
{
public:
    virtual ~LoadableConstant() = default;
};

}   // namespace jclassfile::model
