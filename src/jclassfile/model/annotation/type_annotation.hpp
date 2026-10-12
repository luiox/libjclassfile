// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/TypeAnnotation.java
// 状态：阶段 6 完整实现
// 备注：extends Annotation，叠加 targetInfo + typePath 两个字段。
//       computeLength = 1 (u1 target_type) + targetInfo.computeLength() +
//                       typePath.computeLength() + super.computeLength()。

#pragma once

#include <map>
#include <memory>

#include "jclassfile/model/annotation/annotation.hpp"
#include "jclassfile/model/annotation/target_info.hpp"
#include "jclassfile/model/annotation/type_path.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class TypeAnnotation final : public Annotation
{
public:
    TypeAnnotation(const CpUtf8*                                          type,
                   std::map<const CpUtf8*, std::unique_ptr<ElementValue>> values,
                   std::unique_ptr<TargetInfo> targetInfo, std::unique_ptr<TypePath> typePath)
        : Annotation(type, std::move(values))
        , targetInfo_(std::move(targetInfo))
        , typePath_(std::move(typePath))
    {}

    [[nodiscard]] AnnotationKind kind() const noexcept override
    {
        return AnnotationKind::TypeAnnotation;
    }

    [[nodiscard]] const TargetInfo* getTargetInfo() const noexcept { return targetInfo_.get(); }
    [[nodiscard]] TargetInfo*       getTargetInfo() noexcept { return targetInfo_.get(); }
    void                            setTargetInfo(std::unique_ptr<TargetInfo> targetInfo) noexcept
    {
        targetInfo_ = std::move(targetInfo);
    }

    [[nodiscard]] const TypePath* getTypePath() const noexcept { return typePath_.get(); }
    [[nodiscard]] TypePath*       getTypePath() noexcept { return typePath_.get(); }
    void                          setTypePath(std::unique_ptr<TypePath> typePath) noexcept
    {
        typePath_ = std::move(typePath);
    }

    [[nodiscard]] int computeLength() const noexcept override;

private:
    std::unique_ptr<TargetInfo> targetInfo_;
    std::unique_ptr<TypePath>   typePath_;
};

}   // namespace jclassfile::model
