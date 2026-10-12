// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpMethodRef.java
// 状态：阶段 2 实现

#pragma once

#include "jclassfile/model/constant/const_ref.hpp"

namespace jclassfile::model {

class CpClass;
class CpNameType;

class CpMethodRef : public ConstRef
{
public:
    CpMethodRef(const CpClass* classRef, const CpNameType* nameType);
};

}   // namespace jclassfile::model
