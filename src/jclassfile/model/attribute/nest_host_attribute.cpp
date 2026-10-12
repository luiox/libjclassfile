// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/NestHostAttribute.java

#include "jclassfile/model/attribute/nest_host_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> NestHostAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(hostClass_);
    return set;
}

}   // namespace jclassfile::model
