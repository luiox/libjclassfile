// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpDouble.java
// 状态：阶段 2 实现
// 备注：wide entry。

#pragma once

#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"

namespace jclassfile::model {

class CpDouble : public CpEntry, public LoadableConstant
{
public:
    explicit CpDouble(double value);
    [[nodiscard]] double value() const noexcept { return value_; }
    void                 setValue(double v) noexcept { value_ = v; }

    [[nodiscard]] bool                        isWide() const noexcept override { return true; }
    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isLoadableConstant() const noexcept override { return true; }

private:
    double value_;
};

}   // namespace jclassfile::model
