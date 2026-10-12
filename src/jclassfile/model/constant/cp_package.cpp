// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpPackage.java

#include "jclassfile/model/constant/cp_package.hpp"

#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

CpPackage::CpPackage(const CpUtf8* packageName)
    : CpEntry(CpTag::PACKAGE)
    , packageName_(packageName)
{}

std::vector<const CpEntry*> CpPackage::cpAccesses() const
{
    return {this, packageName_};
}

std::vector<const CpEntry*> CpPackage::getReferences() const
{
    return {packageName_};
}

bool CpPackage::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::PACKAGE) return false;
    return packageName_ == static_cast<const CpPackage&>(other).packageName_;
}

}   // namespace jclassfile::model
