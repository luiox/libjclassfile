// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpDouble.java

#include "jclassfile/model/constant/cp_double.hpp"

namespace jclassfile::model {

CpDouble::CpDouble(double value)
    : CpEntry(CpTag::DOUBLE)
    , value_(value)
{}

std::vector<const CpEntry*> CpDouble::cpAccesses() const
{
    return {this};
}

bool CpDouble::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::DOUBLE) return false;
    return value_ == static_cast<const CpDouble&>(other).value_;
}

}   // namespace jclassfile::model
