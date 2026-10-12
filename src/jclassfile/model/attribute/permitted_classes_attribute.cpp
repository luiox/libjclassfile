// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/PermittedClassesAttribute.java

#include "jclassfile/model/attribute/permitted_classes_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> PermittedClassesAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const CpClass* c : classes_) set.push_back(c);
    return set;
}

}   // namespace jclassfile::model
