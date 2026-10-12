// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/NestMembersAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class NestMembersAttribute : public Attribute
{
public:
    NestMembersAttribute(const CpUtf8* name, std::vector<const CpClass*> memberClasses) noexcept
        : Attribute(name)
        , memberClasses_(std::move(memberClasses))
    {}

    [[nodiscard]] const std::vector<const CpClass*>& memberClasses() const noexcept
    {
        return memberClasses_;
    }
    void setMemberClasses(std::vector<const CpClass*> memberClasses) noexcept
    {
        memberClasses_ = std::move(memberClasses);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override
    {
        return 2 + static_cast<int>(memberClasses_.size()) * 2;
    }

private:
    std::vector<const CpClass*> memberClasses_;
};

}   // namespace jclassfile::model
