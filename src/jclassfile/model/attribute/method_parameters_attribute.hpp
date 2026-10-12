// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/MethodParametersAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class MethodParametersAttribute : public Attribute
{
public:
    class Parameter : public behavior::CpAccessor
    {
    public:
        Parameter(int accessFlags, const CpUtf8* name) noexcept
            : accessFlags_(accessFlags)
            , name_(name)
        {}

        [[nodiscard]] int accessFlags() const noexcept { return accessFlags_; }
        void              setAccessFlags(int accessFlags) noexcept { accessFlags_ = accessFlags; }

        [[nodiscard]] const CpUtf8* name() const noexcept { return name_; }
        void                        setName(const CpUtf8* name) noexcept { name_ = name; }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    private:
        int           accessFlags_;
        const CpUtf8* name_;
    };

    MethodParametersAttribute(const CpUtf8* name, std::vector<Parameter> parameters) noexcept
        : Attribute(name)
        , parameters_(std::move(parameters))
    {}

    [[nodiscard]] const std::vector<Parameter>& parameters() const noexcept { return parameters_; }
    void setParameters(std::vector<Parameter> parameters) noexcept
    {
        parameters_ = std::move(parameters);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override
    {
        return 1 + static_cast<int>(parameters_.size()) * 4;
    }

private:
    std::vector<Parameter> parameters_;
};

}   // namespace jclassfile::model
