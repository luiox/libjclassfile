// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpClass.java
// 状态：阶段 2 实现

#pragma once

#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"

namespace jclassfile::model {

class CpUtf8;

class CpClass : public CpEntry, public LoadableConstant
{
public:
    explicit CpClass(const CpUtf8* name);
    [[nodiscard]] const CpUtf8* name() const noexcept { return name_; }
    void                        setName(const CpUtf8* name) noexcept { name_ = name; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isLoadableConstant() const noexcept override { return true; }

private:
    const CpUtf8* name_;
};

}   // namespace jclassfile::model
