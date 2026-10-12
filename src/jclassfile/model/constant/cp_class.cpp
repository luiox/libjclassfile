// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpClass.java

#include "jclassfile/model/constant/cp_class.hpp"

#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

CpClass::CpClass(const CpUtf8* name)
    : CpEntry(CpTag::CLASS)
    , name_(name)
{}

std::vector<const CpEntry*> CpClass::cpAccesses() const
{
    return {this, name_};
}

bool CpClass::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::CLASS) return false;
    return name_ == static_cast<const CpClass&>(other).name_;
}

}   // namespace jclassfile::model
