// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/SignatureAttribute.java

#include "jclassfile/model/attribute/signature_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> SignatureAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(signature_);
    return set;
}

}   // namespace jclassfile::model
