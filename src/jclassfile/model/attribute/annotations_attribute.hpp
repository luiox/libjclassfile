// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/AnnotationsAttribute.java

#pragma once

#include <memory>
#include <vector>

#include "jclassfile/model/annotation/annotation.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class AnnotationsAttribute : public Attribute
{
public:
    AnnotationsAttribute(const CpUtf8* name, std::vector<std::unique_ptr<Annotation>> annotations,
                         bool visible) noexcept
        : Attribute(name)
        , annotations_(std::move(annotations))
        , visible_(visible)
    {}

    [[nodiscard]] const std::vector<std::unique_ptr<Annotation>>& annotations() const noexcept
    {
        return annotations_;
    }
    void setAnnotations(std::vector<std::unique_ptr<Annotation>> annotations) noexcept
    {
        annotations_ = std::move(annotations);
    }

    [[nodiscard]] bool isVisible() const noexcept { return visible_; }
    void               setVisible(bool visible) noexcept { visible_ = visible; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override;

private:
    std::vector<std::unique_ptr<Annotation>> annotations_;
    bool                                     visible_;
};

}   // namespace jclassfile::model
