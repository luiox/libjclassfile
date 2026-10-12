// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpInterfaceMethodRef.java

#include "jclassfile/model/constant/cp_interface_method_ref.hpp"

#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"

namespace jclassfile::model {

CpInterfaceMethodRef::CpInterfaceMethodRef(const CpClass* classRef, const CpNameType* nameType)
    : ConstRef(CpTag::INTERFACE_METHOD_REF, classRef, nameType)
{}

}   // namespace jclassfile::model
