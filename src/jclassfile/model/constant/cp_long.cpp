// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpLong.java

#include "jclassfile/model/constant/cp_long.hpp"

namespace jclassfile::model {

CpLong::CpLong(ca::i64 value)
    : CpEntry(CpTag::LONG)
    , value_(value)
{}

std::vector<const CpEntry*> CpLong::cpAccesses() const
{
    return {this};
}

bool CpLong::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::LONG) return false;
    return value_ == static_cast<const CpLong&>(other).value_;
}

}   // namespace jclassfile::model
