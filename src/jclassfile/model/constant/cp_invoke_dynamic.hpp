// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpInvokeDynamic.java
// 状态：阶段 2 实现

#pragma once

#include "libca/core/datatype.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"

namespace jclassfile::model {

class CpNameType;

class CpInvokeDynamic : public ConstDynamic
{
public:
    CpInvokeDynamic(ca::u16 bsmIndex, const CpNameType* nameType);
};

}   // namespace jclassfile::model
