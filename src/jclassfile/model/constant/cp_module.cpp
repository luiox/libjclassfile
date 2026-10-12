// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpModule.java

#include "jclassfile/model/constant/cp_module.hpp"

#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

CpModule::CpModule(const CpUtf8* name)
    : CpEntry(CpTag::MODULE)
    , name_(name)
{}

std::vector<const CpEntry*> CpModule::cpAccesses() const
{
    return {this, name_};
}

std::vector<const CpEntry*> CpModule::getReferences() const
{
    return {name_};
}

bool CpModule::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::MODULE) return false;
    return name_ == static_cast<const CpModule&>(other).name_;
}

}   // namespace jclassfile::model
