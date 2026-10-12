// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpDynamic.java

#include "jclassfile/model/constant/cp_dynamic.hpp"

#include "jclassfile/model/constant/cp_name_type.hpp"

namespace jclassfile::model {

CpDynamic::CpDynamic(ca::u16 bsmIndex, const CpNameType* nameType)
    : ConstDynamic(CpTag::DYNAMIC, bsmIndex, nameType)
{}

}   // namespace jclassfile::model
