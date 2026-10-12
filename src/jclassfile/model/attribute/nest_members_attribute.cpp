// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/NestMembersAttribute.java

#include "jclassfile/model/attribute/nest_members_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> NestMembersAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const CpClass* c : memberClasses_) set.push_back(c);
    return set;
}

}   // namespace jclassfile::model
