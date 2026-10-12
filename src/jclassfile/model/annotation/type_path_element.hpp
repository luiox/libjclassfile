// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/TypePathElement.java
// 状态：阶段 6 完整实现
// 备注：原版仅 kind + argIndex 字段。
//       构造时校验：kind != TYPE_ARGUMENT 时 argIndex 必须为 0。

#pragma once

#include <stdexcept>

#include "jclassfile/model/annotation/type_path_kind.hpp"

namespace jclassfile::model {

class TypePathElement
{
public:
    TypePathElement(TypePathKind kind, int argIndex) noexcept
        : kind_(kind)
        , argIndex_(argIndex)
    {
        if (kind != TypePathKind::TYPE_ARGUMENT && argIndex != 0) {
            throw std::invalid_argument(
                "Type path kind was not TYPE_ARGUMENT but gave a non-zero type_argument_index");
        }
    }

    [[nodiscard]] TypePathKind getKind() const noexcept { return kind_; }
    [[nodiscard]] int          getArgIndex() const noexcept { return argIndex_; }

private:
    TypePathKind kind_;
    int          argIndex_;
};

}   // namespace jclassfile::model
