// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ExceptionsAttribute.java

#include "jclassfile/model/attribute/exceptions_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ExceptionsAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const CpClass* c : exceptionTable_) set.push_back(c);
    return set;
}

}   // namespace jclassfile::model
