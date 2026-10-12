// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpInt.java

#include "jclassfile/model/constant/cp_int.hpp"

namespace jclassfile::model {

CpInt::CpInt(ca::i32 value)
    : CpEntry(CpTag::INTEGER)
    , value_(value)
{}

std::vector<const CpEntry*> CpInt::cpAccesses() const
{
    return {this};
}

bool CpInt::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::INTEGER) return false;
    return value_ == static_cast<const CpInt&>(other).value_;
}

}   // namespace jclassfile::model
