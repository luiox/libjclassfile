// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModuleMainClassAttribute.java

#include "jclassfile/model/attribute/module_main_class_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ModuleMainClassAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(mainClass_);
    return set;
}

}   // namespace jclassfile::model
