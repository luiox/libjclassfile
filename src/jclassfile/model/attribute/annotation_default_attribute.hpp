// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/AnnotationDefaultAttribute.java

#pragma once

#include <memory>
#include <vector>

#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class AnnotationDefaultAttribute : public Attribute
{
public:
    AnnotationDefaultAttribute(const CpUtf8* name, std::unique_ptr<ElementValue> elementValue)
        : Attribute(name)
        , elementValue_(std::move(elementValue))
    {}

    [[nodiscard]] const ElementValue* elementValue() const noexcept { return elementValue_.get(); }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override;

private:
    std::unique_ptr<ElementValue> elementValue_;
};

}   // namespace jclassfile::model
