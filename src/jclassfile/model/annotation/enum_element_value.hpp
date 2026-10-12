// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/EnumElementValue.java
// 状态：阶段 6 完整实现

#pragma once

#include <stdexcept>
#include <vector>

#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/annotation/element_value_constants.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class EnumElementValue : public ElementValue
{
public:
    EnumElementValue(char tag, const CpUtf8* type, const CpUtf8* name)
        : ElementValue(tag)
        , type_(type)
        , name_(name)
    {
        if (tag != ElementValueConstants::TAG_ENUM) {
            throw std::invalid_argument("UTF8 element value must have 'e' tag");
        }
    }

    [[nodiscard]] ElementValueKind kind() const noexcept override { return ElementValueKind::Enum; }

    [[nodiscard]] const CpUtf8* getType() const noexcept { return type_; }
    void                        setType(const CpUtf8* type) noexcept { type_ = type; }

    [[nodiscard]] const CpUtf8* getName() const noexcept { return name_; }
    void                        setName(const CpUtf8* name) noexcept { name_ = name; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeLength() const noexcept override
    {
        return 5;
    }   // u1 tag + u2 type + u2 name

private:
    const CpUtf8* type_;
    const CpUtf8* name_;
};

}   // namespace jclassfile::model
