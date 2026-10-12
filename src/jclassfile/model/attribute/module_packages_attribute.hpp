// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModulePackagesAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_package.hpp"

namespace jclassfile::model {

class ModulePackagesAttribute : public Attribute
{
public:
    ModulePackagesAttribute(const CpUtf8* name, std::vector<const CpPackage*> packages) noexcept
        : Attribute(name)
        , packages_(std::move(packages))
    {}

    [[nodiscard]] const std::vector<const CpPackage*>& packages() const noexcept
    {
        return packages_;
    }
    void setPackages(std::vector<const CpPackage*> packages) noexcept
    {
        packages_ = std::move(packages);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override
    {
        return 2 + static_cast<int>(packages_.size()) * 2;
    }

private:
    std::vector<const CpPackage*> packages_;
};

}   // namespace jclassfile::model
