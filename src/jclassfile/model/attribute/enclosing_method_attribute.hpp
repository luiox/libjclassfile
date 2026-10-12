// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/EnclosingMethodAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"

namespace jclassfile::model {

class EnclosingMethodAttribute : public Attribute
{
public:
    EnclosingMethodAttribute(const CpUtf8* name, const CpClass* classEntry,
                             const CpNameType* methodEntry) noexcept
        : Attribute(name)
        , classEntry_(classEntry)
        , methodEntry_(methodEntry)
    {}

    [[nodiscard]] const CpClass* classEntry() const noexcept { return classEntry_; }
    void setClassEntry(const CpClass* classEntry) noexcept { classEntry_ = classEntry; }

    [[nodiscard]] const CpNameType* methodEntry() const noexcept { return methodEntry_; }
    void setMethodEntry(const CpNameType* methodEntry) noexcept { methodEntry_ = methodEntry; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int computeInternalLength() const noexcept override { return 4; }

private:
    const CpClass*    classEntry_;
    const CpNameType* methodEntry_;
};

}   // namespace jclassfile::model
