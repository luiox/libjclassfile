// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ParameterAnnotationsAttribute.java

#include "jclassfile/model/attribute/parameter_annotations_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ParameterAnnotationsAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const auto& list : parameterAnnotations_) {
        for (const auto& a : list) {
            for (const CpEntry* e : a->cpAccesses()) set.push_back(e);
        }
    }
    return set;
}

int ParameterAnnotationsAttribute::computeInternalLength() const noexcept
{
    int len = 1;   // u1: num_parameters
    for (const auto& list : parameterAnnotations_) {
        // u2: num_annotations + annotations
        len += 2;
        for (const auto& a : list) len += a->computeLength();
    }
    return len;
}

}   // namespace jclassfile::model
