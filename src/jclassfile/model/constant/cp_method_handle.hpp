// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpMethodHandle.java
// 状态：阶段 2 实现
// 备注：原版 non-sealed，implements CrossCpReferencing + LoadableConstant。
//       kind 字节：1=GET_FIELD, 2=GET_STATIC, 3=PUT_FIELD, 4=PUT_STATIC,
//                  5=INVOKE_VIRTUAL, 6=INVOKE_STATIC, 7=INVOKE_SPECIAL,
//                  8=NEW_INVOKE_SPECIAL, 9=INVOKE_INTERFACE。

#pragma once

#include <vector>

#include "libca/core/datatype.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cross_cp_referencing.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"

namespace jclassfile::model {

class ConstRef;

class CpMethodHandle : public CpEntry, public CrossCpReferencing, public LoadableConstant
{
public:
    static constexpr ca::i32 REF_GET_FIELD          = 1;
    static constexpr ca::i32 REF_GET_STATIC         = 2;
    static constexpr ca::i32 REF_PUT_FIELD          = 3;
    static constexpr ca::i32 REF_PUT_STATIC         = 4;
    static constexpr ca::i32 REF_INVOKE_VIRTUAL     = 5;
    static constexpr ca::i32 REF_INVOKE_STATIC      = 6;
    static constexpr ca::i32 REF_INVOKE_SPECIAL     = 7;
    static constexpr ca::i32 REF_NEW_INVOKE_SPECIAL = 8;
    static constexpr ca::i32 REF_INVOKE_INTERFACE   = 9;

    CpMethodHandle(ca::u8 kind, const ConstRef* reference);

    [[nodiscard]] ca::u8 kind() const noexcept { return kind_; }
    void                 setKind(ca::u8 kind) noexcept { kind_ = kind; }

    [[nodiscard]] const ConstRef* reference() const noexcept { return reference_; }
    void setReference(const ConstRef* reference) noexcept { reference_ = reference; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] std::vector<const CpEntry*> getReferences() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

    [[nodiscard]] bool isLoadableConstant() const noexcept override { return true; }
    [[nodiscard]] bool isCrossReferencing() const noexcept override { return true; }
    [[nodiscard]] const CrossCpReferencing* asCrossReferencing() const noexcept override
    {
        return this;
    }

private:
    const ConstRef* reference_;
    ca::u8          kind_;
};

}   // namespace jclassfile::model
