// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpEntry.java
// 状态：阶段 2 实现
// 备注：原版 sealed abstract permits 14 concrete classes + 1 interface。
//       C++ 用 abstract class + 注释说明 "sealed equivalent"。
//       无 RTTI：用 tag() + isXxx() 虚方法替代 dynamic_cast 做类型分派与 marker 检查。

#pragma once

#include <vector>

#include "libca/core/datatype.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant_pool_constants.hpp"

namespace jclassfile::model::behavior {
class CpAccessor;
}

namespace jclassfile::model {

class CrossCpReferencing;

class CpEntry : public behavior::CpAccessor
{
public:
    explicit CpEntry(CpTag tag) noexcept
        : tag_(tag)
    {}
    virtual ~CpEntry() = default;

    CpEntry(const CpEntry&)            = delete;
    CpEntry& operator=(const CpEntry&) = delete;

    [[nodiscard]] CpTag        tag() const noexcept { return tag_; }
    [[nodiscard]] ca::u16      index() const;
    void                       setIndex(ca::u16 index) noexcept;
    [[nodiscard]] bool         isInPool() const noexcept { return index_ > 0; }
    [[nodiscard]] virtual bool isWide() const noexcept { return false; }

    // marker 检查（替代 Java instanceof，对应 marker interface 检查）
    [[nodiscard]] virtual bool isConstRef() const noexcept { return false; }
    [[nodiscard]] virtual bool isConstDynamic() const noexcept { return false; }
    [[nodiscard]] virtual bool isCrossReferencing() const noexcept { return false; }
    [[nodiscard]] virtual bool isLoadableConstant() const noexcept { return false; }

    // 若 isCrossReferencing()==true，返回 CrossCpReferencing*；否则 nullptr。
    [[nodiscard]] virtual const CrossCpReferencing* asCrossReferencing() const noexcept
    {
        return nullptr;
    }

    [[nodiscard]] virtual std::vector<const CpEntry*> cpAccesses() const = 0;

    [[nodiscard]] virtual bool equals(const CpEntry& other) const = 0;

private:
    CpTag   tag_;
    ca::u16 index_ = 0;
};

}   // namespace jclassfile::model
