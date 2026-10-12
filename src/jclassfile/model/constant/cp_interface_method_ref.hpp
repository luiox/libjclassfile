// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpInterfaceMethodRef.java
// 状态：阶段 2 实现

#pragma once

#include "jclassfile/model/constant/const_ref.hpp"

namespace jclassfile::model {

class CpClass;
class CpNameType;

class CpInterfaceMethodRef : public ConstRef
{
public:
    CpInterfaceMethodRef(const CpClass* classRef, const CpNameType* nameType);
};

}   // namespace jclassfile::model
