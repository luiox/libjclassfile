// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/DefaultAttribute.java

#pragma once

#include <cstdint>
#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"

namespace jclassfile::model {

class DefaultAttribute : public Attribute
{
public:
    DefaultAttribute(const CpUtf8* name, std::vector<ca::u8> data) noexcept
        : Attribute(name)
        , data_(std::move(data))
    {}

    [[nodiscard]] const std::vector<ca::u8>& data() const noexcept { return data_; }
    void setData(std::vector<ca::u8> data) noexcept { data_ = std::move(data); }

    [[nodiscard]] int computeInternalLength() const noexcept override
    {
        return static_cast<int>(data_.size());
    }

    [[nodiscard]] AttrKind kind() const noexcept override { return AttrKind::Default; }
    [[nodiscard]] bool     isDefaultAttribute() const noexcept override { return true; }

private:
    std::vector<ca::u8> data_;
};

}   // namespace jclassfile::model
