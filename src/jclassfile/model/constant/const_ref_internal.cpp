// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/ConstRefInternal.java

#include "jclassfile/model/constant/const_ref_internal.hpp"

#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"

namespace jclassfile::model {

// 原版 super(-1, ...)，tag = -1 标记 internal-only。
// C++ 用 CpTag::INTERNAL (= 0) 表达同样语义，ConstPool 内部按 tag <= 0 判定 padding。
ConstRefInternal::ConstRefInternal(const CpClass* classRef, const CpNameType* nameType)
    : ConstRef(CpTag::INTERNAL, classRef, nameType)
{}

}   // namespace jclassfile::model
