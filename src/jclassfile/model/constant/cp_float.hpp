// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpFloat.java
// 状态：阶段 2 实现

#pragma once

#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"

namespace jclassfile::model {

class CpFloat : public CpEntry, public LoadableConstant
{
public:
    explicit CpFloat(float value);
    [[nodiscard]] float value() const noexcept { return value_; }
    void                setValue(float v) noexcept { value_ = v; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isLoadableConstant() const noexcept override { return true; }

private:
    float value_;
};

}   // namespace jclassfile::model
