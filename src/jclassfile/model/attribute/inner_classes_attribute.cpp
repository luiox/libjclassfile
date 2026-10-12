// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/InnerClassesAttribute.java

#include "jclassfile/model/attribute/inner_classes_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> InnerClassesAttribute::InnerClass::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    if (outerClassInfo_ != nullptr) set.push_back(outerClassInfo_);
    if (innerClassInfo_ != nullptr) set.push_back(innerClassInfo_);
    if (innerName_ != nullptr) set.push_back(innerName_);
    return set;
}

std::vector<const CpEntry*> InnerClassesAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const InnerClass& ic : innerClasses_) {
        for (const CpEntry* e : ic.cpAccesses()) set.push_back(e);
    }
    return set;
}

}   // namespace jclassfile::model
