// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/EnclosingMethodAttribute.java

#include "jclassfile/model/attribute/enclosing_method_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> EnclosingMethodAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(classEntry_);
    if (methodEntry_ != nullptr) set.push_back(methodEntry_);
    return set;
}

}   // namespace jclassfile::model
