// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/AnnotationElementValue.java
// 状态：阶段 6 完整实现
// 备注：嵌套 annotation。tag 强制为 '@'。
//       持有 std::unique_ptr<Annotation>（与原版持有非空 Annotation 引用一致）。

#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include "jclassfile/model/annotation/annotation.hpp"
#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/annotation/element_value_constants.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class AnnotationElementValue : public ElementValue
{
public:
    AnnotationElementValue(char tag, std::unique_ptr<Annotation> annotation)
        : ElementValue(tag)
        , annotation_(std::move(annotation))
    {
        if (tag != ElementValueConstants::TAG_ANNOTATION) {
            throw std::invalid_argument("Annotation element value must have '@' tag");
        }
    }

    [[nodiscard]] ElementValueKind kind() const noexcept override
    {
        return ElementValueKind::Annotation;
    }

    [[nodiscard]] const Annotation* getAnnotation() const noexcept { return annotation_.get(); }
    [[nodiscard]] Annotation*       getAnnotation() noexcept { return annotation_.get(); }
    void                            setAnnotation(std::unique_ptr<Annotation> annotation) noexcept
    {
        annotation_ = std::move(annotation);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeLength() const noexcept override;

private:
    std::unique_ptr<Annotation> annotation_;
};

}   // namespace jclassfile::model
