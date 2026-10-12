// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/TypePath.java
// 状态：阶段 6 完整实现
// 备注：computeLength = 1 (u1 path_length) + 2 * size（每个 path element = kind + argIndex）。

#pragma once

#include <vector>

#include "jclassfile/model/annotation/type_path_element.hpp"

namespace jclassfile::model {

class TypePath
{
public:
    explicit TypePath(std::vector<TypePathElement> path) noexcept
        : path_(std::move(path))
    {}

    [[nodiscard]] const std::vector<TypePathElement>& getPath() const noexcept { return path_; }
    std::vector<TypePathElement>&                     getPath() noexcept { return path_; }

    [[nodiscard]] int computeLength() const noexcept
    {
        return 1 + (2 * static_cast<int>(path_.size()));
    }

private:
    std::vector<TypePathElement> path_;
};

}   // namespace jclassfile::model
