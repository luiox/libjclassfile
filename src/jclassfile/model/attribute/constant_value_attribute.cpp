// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ConstantValueAttribute.java

#include "jclassfile/model/attribute/constant_value_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ConstantValueAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(constantValue_);
    return set;
}

}   // namespace jclassfile::model
