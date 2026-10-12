// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ParameterAnnotationsAttribute.java

#pragma once

#include <memory>
#include <vector>

#include "jclassfile/model/annotation/annotation.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class ParameterAnnotationsAttribute : public Attribute
{
public:
    ParameterAnnotationsAttribute(
        const CpUtf8*                                         name,
        std::vector<std::vector<std::unique_ptr<Annotation>>> parameterAnnotations,
        bool                                                  visible) noexcept
        : Attribute(name)
        , parameterAnnotations_(std::move(parameterAnnotations))
        , visible_(visible)
    {}

    [[nodiscard]] const std::vector<std::vector<std::unique_ptr<Annotation>>>&
    parameterAnnotations() const noexcept
    {
        return parameterAnnotations_;
    }
    void setParameterAnnotations(
        std::vector<std::vector<std::unique_ptr<Annotation>>> parameterAnnotations) noexcept
    {
        parameterAnnotations_ = std::move(parameterAnnotations);
    }

    [[nodiscard]] bool isVisible() const noexcept { return visible_; }
    void               setVisible(bool visible) noexcept { visible_ = visible; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override;

private:
    std::vector<std::vector<std::unique_ptr<Annotation>>> parameterAnnotations_;
    bool                                                  visible_;
};

}   // namespace jclassfile::model
