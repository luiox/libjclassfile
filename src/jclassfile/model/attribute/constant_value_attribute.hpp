// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ConstantValueAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class ConstantValueAttribute : public Attribute
{
public:
    ConstantValueAttribute(const CpUtf8* name, const CpEntry* constantValue) noexcept
        : Attribute(name)
        , constantValue_(constantValue)
    {}

    [[nodiscard]] const CpEntry* constantValue() const noexcept { return constantValue_; }
    void                         setConstantValueIndex(const CpEntry* constantValue) noexcept
    {
        constantValue_ = constantValue;
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int computeInternalLength() const noexcept override { return 2; }

private:
    const CpEntry* constantValue_;
};

}   // namespace jclassfile::model
