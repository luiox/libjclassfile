// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpInt.java
// 状态：阶段 2 实现

#pragma once

#include <vector>

#include "libca/core/datatype.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"

namespace jclassfile::model {

class CpInt : public CpEntry, public LoadableConstant
{
public:
    explicit CpInt(ca::i32 value);
    [[nodiscard]] ca::i32 value() const noexcept { return value_; }
    void                  setValue(ca::i32 v) noexcept { value_ = v; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isLoadableConstant() const noexcept override { return true; }

private:
    ca::i32 value_;
};

}   // namespace jclassfile::model
