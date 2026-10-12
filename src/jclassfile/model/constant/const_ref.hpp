// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/ConstRef.java
// 状态：阶段 2 实现
// 备注：原版 sealed abstract permits 4 子类（ConstRefInternal / CpFieldRef / CpMethodRef /
// CpInterfaceMethodRef）。
//       C++ 用 protected 构造函数 + 注释说明 "sealed abstract equivalent"（C++ 无 sealed 关键字）。

#pragma once

#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cross_cp_referencing.hpp"

namespace jclassfile::model {

class CpClass;
class CpNameType;

class ConstRef : public CpEntry, public CrossCpReferencing
{
public:
    ConstRef(CpTag tag, const CpClass* classRef, const CpNameType* nameType);

    [[nodiscard]] const CpClass*    classRef() const noexcept { return classRef_; }
    [[nodiscard]] const CpNameType* nameType() const noexcept { return nameType_; }
    void setClassRef(const CpClass* classRef) noexcept { classRef_ = classRef; }
    void setNameType(const CpNameType* nameType) noexcept { nameType_ = nameType; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] std::vector<const CpEntry*> getReferences() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isConstRef() const noexcept override { return true; }
    [[nodiscard]] bool isCrossReferencing() const noexcept override { return true; }
    [[nodiscard]] const CrossCpReferencing* asCrossReferencing() const noexcept override
    {
        return this;
    }

protected:
    const CpClass*    classRef_;
    const CpNameType* nameType_;
};

}   // namespace jclassfile::model
