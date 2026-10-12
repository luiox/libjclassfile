// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ExceptionsAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_class.hpp"

namespace jclassfile::model {

class ExceptionsAttribute : public Attribute
{
public:
    ExceptionsAttribute(const CpUtf8* name, std::vector<const CpClass*> exceptionTable) noexcept
        : Attribute(name)
        , exceptionTable_(std::move(exceptionTable))
    {}

    [[nodiscard]] const std::vector<const CpClass*>& exceptionTable() const noexcept
    {
        return exceptionTable_;
    }
    void setExceptionTable(std::vector<const CpClass*> exceptionTable) noexcept
    {
        exceptionTable_ = std::move(exceptionTable);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override
    {
        return 2 + static_cast<int>(exceptionTable_.size()) * 2;
    }

private:
    std::vector<const CpClass*> exceptionTable_;
};

}   // namespace jclassfile::model
