// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpPackage.java
// 状态：阶段 2 实现
// 备注：原版 non-sealed，implements CrossCpReferencing（无 LoadableConstant）。

#pragma once

#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cross_cp_referencing.hpp"

namespace jclassfile::model {

class CpUtf8;

class CpPackage : public CpEntry, public CrossCpReferencing
{
public:
    explicit CpPackage(const CpUtf8* packageName);

    [[nodiscard]] const CpUtf8* packageName() const noexcept { return packageName_; }
    void setPackageName(const CpUtf8* packageName) noexcept { packageName_ = packageName; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] std::vector<const CpEntry*> getReferences() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isCrossReferencing() const noexcept override { return true; }
    [[nodiscard]] const CrossCpReferencing* asCrossReferencing() const noexcept override
    {
        return this;
    }

private:
    const CpUtf8* packageName_;
};

}   // namespace jclassfile::model
