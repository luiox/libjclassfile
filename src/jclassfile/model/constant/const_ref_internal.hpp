// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/ConstRefInternal.java
// 状态：阶段 2 实现
// 备注：原版 non-sealed，ConstPool 内部使用，外部用户不应见到。
//       继承 ConstRef + 实现 CpInternal marker（has-a 关系用 C++ 多继承模拟）。

#pragma once

#include "jclassfile/model/constant/const_ref.hpp"
#include "jclassfile/model/constant/cp_internal.hpp"

namespace jclassfile::model {

class CpClass;
class CpNameType;

class ConstRefInternal : public ConstRef, public CpInternal
{
public:
    ConstRefInternal(const CpClass* classRef, const CpNameType* nameType);
};

}   // namespace jclassfile::model
