// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/AttributeContext.java
// 状态：阶段 1 实现
// 备注：原版是 Java record，只有 type / memberAccess 两个字段。name() 返回 type.name()。
//       isAttribute() 实现为 type == ATTRIBUTE || isRecordComponent()，与原版逐字对齐。

#pragma once

#include "attribute_holder_type.hpp"

namespace jclassfile::io {

class AttributeContext
{
public:
    constexpr AttributeContext(AttributeHolderType type, int memberAccess) noexcept
        : type_(type)
        , memberAccess_(memberAccess)
    {}

    [[nodiscard]] constexpr AttributeHolderType type() const noexcept { return type_; }

    [[nodiscard]] constexpr int memberAccess() const noexcept { return memberAccess_; }

    [[nodiscard]] const char* name() const noexcept;

    [[nodiscard]] constexpr bool isClass() const noexcept
    {
        return type_ == AttributeHolderType::CLASS;
    }

    [[nodiscard]] constexpr bool isField() const noexcept
    {
        return type_ == AttributeHolderType::FIELD;
    }

    [[nodiscard]] constexpr bool isRecordComponent() const noexcept
    {
        return type_ == AttributeHolderType::RECORD_COMPONENT;
    }

    [[nodiscard]] constexpr bool isMethod() const noexcept
    {
        return type_ == AttributeHolderType::METHOD;
    }

    [[nodiscard]] constexpr bool isAttribute() const noexcept
    {
        return type_ == AttributeHolderType::ATTRIBUTE || isRecordComponent();
    }

private:
    AttributeHolderType type_;
    int                 memberAccess_;
};

}   // namespace jclassfile::io
