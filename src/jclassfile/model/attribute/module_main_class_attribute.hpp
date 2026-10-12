// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModuleMainClassAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class ModuleMainClassAttribute : public Attribute
{
public:
    ModuleMainClassAttribute(const CpUtf8* name, const CpClass* mainClass) noexcept
        : Attribute(name)
        , mainClass_(mainClass)
    {}

    [[nodiscard]] const CpClass* mainClass() const noexcept { return mainClass_; }
    void setMainClass(const CpClass* mainClass) noexcept { mainClass_ = mainClass; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int computeInternalLength() const noexcept override { return 2; }

private:
    const CpClass* mainClass_;
};

}   // namespace jclassfile::model
