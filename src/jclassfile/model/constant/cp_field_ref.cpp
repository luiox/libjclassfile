// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpFieldRef.java

#include "jclassfile/model/constant/cp_field_ref.hpp"

#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"

namespace jclassfile::model {

CpFieldRef::CpFieldRef(const CpClass* classRef, const CpNameType* nameType)
    : ConstRef(CpTag::FIELD_REF, classRef, nameType)
{}

}   // namespace jclassfile::model
