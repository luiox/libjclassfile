// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/SyntheticAttribute.java

#pragma once

#include "jclassfile/model/attribute/attribute.hpp"

namespace jclassfile::model {

class SyntheticAttribute : public Attribute
{
public:
    explicit SyntheticAttribute(const CpUtf8* name) noexcept
        : Attribute(name)
    {}

    [[nodiscard]] int computeInternalLength() const noexcept override { return 0; }
};

}   // namespace jclassfile::model
