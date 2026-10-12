// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpNameType.java
// 状态：阶段 2 实现

#pragma once

#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class CpUtf8;

class CpNameType : public CpEntry
{
public:
    CpNameType(const CpUtf8* name, const CpUtf8* type);
    [[nodiscard]] const CpUtf8* name() const noexcept { return name_; }
    [[nodiscard]] const CpUtf8* type() const noexcept { return type_; }
    void                        setName(const CpUtf8* name) noexcept { name_ = name; }
    void                        setType(const CpUtf8* type) noexcept { type_ = type; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

private:
    const CpUtf8* name_;
    const CpUtf8* type_;
};

}   // namespace jclassfile::model
