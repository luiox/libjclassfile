// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/NestHostAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class NestHostAttribute : public Attribute
{
public:
    NestHostAttribute(const CpUtf8* name, const CpClass* hostClass) noexcept
        : Attribute(name)
        , hostClass_(hostClass)
    {}

    [[nodiscard]] const CpClass* hostClass() const noexcept { return hostClass_; }
    void setHostClass(const CpClass* hostClass) noexcept { hostClass_ = hostClass; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int computeInternalLength() const noexcept override { return 2; }

private:
    const CpClass* hostClass_;
};

}   // namespace jclassfile::model
