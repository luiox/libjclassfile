// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpDynamic.java
// 状态：阶段 2 实现
// 备注：原版 non-sealed，implements LoadableConstant。

#pragma once

#include "libca/core/datatype.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"

namespace jclassfile::model {

class CpNameType;

class CpDynamic : public ConstDynamic, public LoadableConstant
{
public:
    CpDynamic(ca::u16 bsmIndex, const CpNameType* nameType);

    [[nodiscard]] bool isLoadableConstant() const noexcept override { return true; }
};

}   // namespace jclassfile::model
