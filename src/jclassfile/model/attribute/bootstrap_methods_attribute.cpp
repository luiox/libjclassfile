// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/BootstrapMethodsAttribute.java

#include "jclassfile/model/attribute/bootstrap_methods_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> BootstrapMethodsAttribute::BootstrapMethod::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    set.push_back(bsmMethodRef_);
    for (const CpEntry* a : args_) set.push_back(a);
    return set;
}

std::vector<const CpEntry*> BootstrapMethodsAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const BootstrapMethod& bsm : bootstrapMethods_) {
        for (const CpEntry* e : bsm.cpAccesses()) set.push_back(e);
    }
    return set;
}

}   // namespace jclassfile::model
