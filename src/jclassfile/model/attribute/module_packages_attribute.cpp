// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModulePackagesAttribute.java

#include "jclassfile/model/attribute/module_packages_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ModulePackagesAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const CpPackage* p : packages_) set.push_back(p);
    return set;
}

}   // namespace jclassfile::model
