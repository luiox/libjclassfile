// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpString.java

#include "jclassfile/model/constant/cp_string.hpp"

#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

CpString::CpString(const CpUtf8* value)
    : CpEntry(CpTag::STRING)
    , value_(value)
{}

std::vector<const CpEntry*> CpString::cpAccesses() const
{
    return {this, value_};
}

bool CpString::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::STRING) return false;
    return value_ == static_cast<const CpString&>(other).value_;
}

}   // namespace jclassfile::model
