// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpInvokeDynamic.java

#include "jclassfile/model/constant/cp_invoke_dynamic.hpp"

#include "jclassfile/model/constant/cp_name_type.hpp"

namespace jclassfile::model {

CpInvokeDynamic::CpInvokeDynamic(ca::u16 bsmIndex, const CpNameType* nameType)
    : ConstDynamic(CpTag::INVOKE_DYNAMIC, bsmIndex, nameType)
{}

}   // namespace jclassfile::model
