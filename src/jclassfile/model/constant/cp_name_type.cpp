// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpNameType.java

#include "jclassfile/model/constant/cp_name_type.hpp"

#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

CpNameType::CpNameType(const CpUtf8* name, const CpUtf8* type)
    : CpEntry(CpTag::NAME_TYPE)
    , name_(name)
    , type_(type)
{}

std::vector<const CpEntry*> CpNameType::cpAccesses() const
{
    return {this, name_, type_};
}

bool CpNameType::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::NAME_TYPE) return false;
    const auto& o = static_cast<const CpNameType&>(other);
    return name_ == o.name_ && type_ == o.type_;
}

}   // namespace jclassfile::model
