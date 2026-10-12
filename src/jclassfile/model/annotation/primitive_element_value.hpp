// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/PrimitiveElementValue.java
// 状态：阶段 6 完整实现
// 备注：原版不强制 tag 校验（primitives 包含 B/C/D/F/I/J/S/Z 多个）。
//       C++ 沿用原版：不校验 tag，由调用方保证。

#pragma once

#include <vector>

#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class PrimitiveElementValue : public ElementValue
{
public:
    PrimitiveElementValue(char tag, const CpEntry* value)
        : ElementValue(tag)
        , value_(value)
    {}

    [[nodiscard]] ElementValueKind kind() const noexcept override
    {
        return ElementValueKind::Primitive;
    }

    [[nodiscard]] const CpEntry* getValue() const noexcept { return value_; }
    void                         setValue(const CpEntry* value) noexcept { value_ = value; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeLength() const noexcept override
    {
        return 3;
    }   // u1 tag + u2 value_index

private:
    const CpEntry* value_;
};

}   // namespace jclassfile::model
