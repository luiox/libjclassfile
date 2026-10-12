// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModuleTargetAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class ModuleTargetAttribute : public Attribute
{
public:
    ModuleTargetAttribute(const CpUtf8* name, const CpUtf8* platformName) noexcept
        : Attribute(name)
        , platformName_(platformName)
    {}

    [[nodiscard]] const CpUtf8* platformName() const noexcept { return platformName_; }
    void setPlatformName(const CpUtf8* platformName) noexcept { platformName_ = platformName; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int computeInternalLength() const noexcept override { return 2; }

private:
    const CpUtf8* platformName_;
};

}   // namespace jclassfile::model
