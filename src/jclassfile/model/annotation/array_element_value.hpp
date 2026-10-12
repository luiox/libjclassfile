// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/ArrayElementValue.java
// 状态：阶段 6 完整实现

#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/annotation/element_value_constants.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class ArrayElementValue : public ElementValue
{
public:
    ArrayElementValue(char tag, std::vector<std::unique_ptr<ElementValue>> array)
        : ElementValue(tag)
        , array_(std::move(array))
    {
        if (tag != ElementValueConstants::TAG_ARRAY) {
            throw std::invalid_argument("Array element value must have '[' tag");
        }
    }

    [[nodiscard]] ElementValueKind kind() const noexcept override
    {
        return ElementValueKind::Array;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<ElementValue>>& getArray() const noexcept
    {
        return array_;
    }
    std::vector<std::unique_ptr<ElementValue>>& getArray() noexcept { return array_; }
    void setArray(std::vector<std::unique_ptr<ElementValue>> array) noexcept
    {
        array_ = std::move(array);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeLength() const noexcept override;

private:
    std::vector<std::unique_ptr<ElementValue>> array_;
};

}   // namespace jclassfile::model
