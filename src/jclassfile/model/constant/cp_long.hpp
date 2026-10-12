// 原版：ref_samples/CAFED00D/core/src/main/java/software/colep/cafedude/classfile/constant/CpLong.java
// 状态：阶段 2 实现
// 备注：wide entry，占两个 slot，isWide() 返回 true。

#pragma once

#include <vector>

#include "libca/core/datatype.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"

namespace jclassfile::model {

class CpLong : public CpEntry, public LoadableConstant
{
public:
    explicit CpLong(ca::i64 value);
    [[nodiscard]] ca::i64 value() const noexcept { return value_; }
    void                  setValue(ca::i64 v) noexcept { value_ = v; }

    [[nodiscard]] bool                        isWide() const noexcept override { return true; }
    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isLoadableConstant() const noexcept override { return true; }

private:
    ca::i64 value_;
};

}   // namespace jclassfile::model
