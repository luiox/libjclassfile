// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpMethodType.java
// 状态：阶段 2 实现
// 备注：原版 non-sealed，implements CrossCpReferencing + LoadableConstant。

#pragma once

#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cross_cp_referencing.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"

namespace jclassfile::model {

class CpUtf8;

class CpMethodType : public CpEntry, public CrossCpReferencing, public LoadableConstant
{
public:
    explicit CpMethodType(const CpUtf8* descriptor);

    [[nodiscard]] const CpUtf8* descriptor() const noexcept { return descriptor_; }
    void setDescriptor(const CpUtf8* descriptor) noexcept { descriptor_ = descriptor; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] std::vector<const CpEntry*> getReferences() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isLoadableConstant() const noexcept override { return true; }
    [[nodiscard]] bool isCrossReferencing() const noexcept override { return true; }
    [[nodiscard]] const CrossCpReferencing* asCrossReferencing() const noexcept override
    {
        return this;
    }

private:
    const CpUtf8* descriptor_;
};

}   // namespace jclassfile::model
