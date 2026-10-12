// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpFloat.java

#include "jclassfile/model/constant/cp_float.hpp"

namespace jclassfile::model {

CpFloat::CpFloat(float value)
    : CpEntry(CpTag::FLOAT)
    , value_(value)
{}

std::vector<const CpEntry*> CpFloat::cpAccesses() const
{
    return {this};
}

bool CpFloat::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::FLOAT) return false;
    return value_ == static_cast<const CpFloat&>(other).value_;
}

}   // namespace jclassfile::model
