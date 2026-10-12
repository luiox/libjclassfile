// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpMethodType.java

#include "jclassfile/model/constant/cp_method_type.hpp"

#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

CpMethodType::CpMethodType(const CpUtf8* descriptor)
    : CpEntry(CpTag::METHOD_TYPE)
    , descriptor_(descriptor)
{}

std::vector<const CpEntry*> CpMethodType::cpAccesses() const
{
    return {this, descriptor_};
}

std::vector<const CpEntry*> CpMethodType::getReferences() const
{
    return {descriptor_};
}

bool CpMethodType::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::METHOD_TYPE) return false;
    return descriptor_ == static_cast<const CpMethodType&>(other).descriptor_;
}

}   // namespace jclassfile::model
