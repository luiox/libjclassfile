// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpModule.java
// 状态：阶段 2 实现
// 备注：原版 non-sealed，implements CrossCpReferencing（无 LoadableConstant）。

#pragma once

#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cross_cp_referencing.hpp"

namespace jclassfile::model {

class CpUtf8;

class CpModule : public CpEntry, public CrossCpReferencing
{
public:
    explicit CpModule(const CpUtf8* name);

    [[nodiscard]] const CpUtf8* name() const noexcept { return name_; }
    void                        setName(const CpUtf8* name) noexcept { name_ = name; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] std::vector<const CpEntry*> getReferences() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isCrossReferencing() const noexcept override { return true; }
    [[nodiscard]] const CrossCpReferencing* asCrossReferencing() const noexcept override
    {
        return this;
    }

private:
    const CpUtf8* name_;
};

}   // namespace jclassfile::model
