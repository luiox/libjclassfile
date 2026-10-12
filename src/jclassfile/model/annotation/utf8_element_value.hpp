// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/Utf8ElementValue.java
// 状态：阶段 6 完整实现
// 备注：原版要求 tag == 's'，否则抛 IllegalArgumentException。
//       C++ 沿用相同校验（抛 std::invalid_argument）。

#pragma once

#include <stdexcept>
#include <vector>

#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/annotation/element_value_constants.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class Utf8ElementValue : public ElementValue
{
public:
    Utf8ElementValue(char tag, const CpUtf8* value)
        : ElementValue(tag)
        , value_(value)
    {
        if (tag != ElementValueConstants::TAG_STRING) {
            throw std::invalid_argument("UTF8 element value must have 's' tag");
        }
    }

    [[nodiscard]] ElementValueKind kind() const noexcept override { return ElementValueKind::Utf8; }

    [[nodiscard]] const CpUtf8* getValue() const noexcept { return value_; }
    void                        setValue(const CpUtf8* value) noexcept { value_ = value; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeLength() const noexcept override
    {
        return 3;
    }   // u1 tag + u2 utf8_index

private:
    const CpUtf8* value_;
};

}   // namespace jclassfile::model
