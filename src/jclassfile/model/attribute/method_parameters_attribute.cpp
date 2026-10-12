// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/MethodParametersAttribute.java

#include "jclassfile/model/attribute/method_parameters_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> MethodParametersAttribute::Parameter::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    if (name_ != nullptr) set.push_back(name_);
    return set;
}

std::vector<const CpEntry*> MethodParametersAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const Parameter& p : parameters_) {
        for (const CpEntry* ce : p.cpAccesses()) set.push_back(ce);
    }
    return set;
}

}   // namespace jclassfile::model
