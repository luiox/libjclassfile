// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModuleTargetAttribute.java

#include "jclassfile/model/attribute/module_target_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ModuleTargetAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(platformName_);
    return set;
}

}   // namespace jclassfile::model
