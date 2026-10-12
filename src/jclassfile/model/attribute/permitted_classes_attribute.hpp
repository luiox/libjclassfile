// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/PermittedClassesAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class PermittedClassesAttribute : public Attribute
{
public:
    PermittedClassesAttribute(const CpUtf8* name, std::vector<const CpClass*> classes) noexcept
        : Attribute(name)
        , classes_(std::move(classes))
    {}

    [[nodiscard]] const std::vector<const CpClass*>& classes() const noexcept { return classes_; }
    void setClasses(std::vector<const CpClass*> classes) noexcept { classes_ = std::move(classes); }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override
    {
        return 2 + static_cast<int>(classes_.size()) * 2;
    }

private:
    std::vector<const CpClass*> classes_;
};

}   // namespace jclassfile::model
