// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/ClassElementValue.java
// 状态：阶段 6 完整实现

#pragma once

#include <stdexcept>
#include <vector>

#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/annotation/element_value_constants.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class ClassElementValue : public ElementValue
{
public:
    ClassElementValue(char tag, const CpUtf8* classEntry)
        : ElementValue(tag)
        , classEntry_(classEntry)
    {
        if (tag != ElementValueConstants::TAG_CLASS) {
            throw std::invalid_argument("Class element value must have 'c' tag");
        }
    }

    [[nodiscard]] ElementValueKind kind() const noexcept override
    {
        return ElementValueKind::Class;
    }

    [[nodiscard]] const CpUtf8* getClassEntry() const noexcept { return classEntry_; }
    void setClassEntry(const CpUtf8* classEntry) noexcept { classEntry_ = classEntry; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeLength() const noexcept override
    {
        return 3;
    }   // u1 tag + u2 class_index

private:
    const CpUtf8* classEntry_;
};

}   // namespace jclassfile::model
