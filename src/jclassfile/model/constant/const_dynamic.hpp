// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/ConstDynamic.java
// 状态：阶段 2 实现
// 备注：原版 sealed abstract permits 2 子类（CpDynamic / CpInvokeDynamic）。
//       C++ 用 protected 构造函数 + 注释说明 "sealed abstract equivalent"（C++ 无 sealed 关键字）。

#pragma once

#include <vector>

#include "libca/core/datatype.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cross_cp_referencing.hpp"

namespace jclassfile::model {

class CpNameType;

class ConstDynamic : public CpEntry, public CrossCpReferencing
{
public:
    ConstDynamic(CpTag tag, ca::u16 bsmIndex, const CpNameType* nameType);

    [[nodiscard]] ca::u16 bsmIndex() const noexcept { return bsmIndex_; }
    void                  setBsmIndex(ca::u16 bsmIndex) noexcept { bsmIndex_ = bsmIndex; }

    [[nodiscard]] const CpNameType* nameType() const noexcept { return nameType_; }
    void setNameType(const CpNameType* nameType) noexcept { nameType_ = nameType; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] std::vector<const CpEntry*> getReferences() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isConstDynamic() const noexcept override { return true; }
    [[nodiscard]] bool isCrossReferencing() const noexcept override { return true; }
    [[nodiscard]] const CrossCpReferencing* asCrossReferencing() const noexcept override
    {
        return this;
    }

protected:
    ca::u16           bsmIndex_;
    const CpNameType* nameType_;
};

}   // namespace jclassfile::model
