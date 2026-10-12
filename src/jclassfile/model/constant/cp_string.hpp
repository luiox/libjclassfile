// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpString.java
// 状态：阶段 2 实现

#pragma once

#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"

namespace jclassfile::model {

class CpUtf8;

class CpString : public CpEntry, public LoadableConstant
{
public:
    explicit CpString(const CpUtf8* value);
    [[nodiscard]] const CpUtf8* value() const noexcept { return value_; }
    void                        setValue(const CpUtf8* value) noexcept { value_ = value; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isLoadableConstant() const noexcept override { return true; }

private:
    const CpUtf8* value_;
};

}   // namespace jclassfile::model
