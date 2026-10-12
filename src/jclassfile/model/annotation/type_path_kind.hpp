// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/TypePathKind.java
// 状态：阶段 6 完整实现
// 备注：4 个枚举值（ARRAY_DEEPER / NESTED_DEEPER / WILDCARD_BOUND / TYPE_ARGUMENT）。
//       fromValue(int) 静态方法与原版 switch 一致。

#pragma once

#include <stdexcept>
#include <string>

namespace jclassfile::model {

enum class TypePathKind
{
    ARRAY_DEEPER   = 0,
    NESTED_DEEPER  = 1,
    WILDCARD_BOUND = 2,
    TYPE_ARGUMENT  = 3,
};

inline int typePathKindValue(TypePathKind k) noexcept
{
    return static_cast<int>(k);
}

inline TypePathKind typePathKindFromValue(int value)
{
    switch (value) {
    case 0: return TypePathKind::ARRAY_DEEPER;
    case 1: return TypePathKind::NESTED_DEEPER;
    case 2: return TypePathKind::WILDCARD_BOUND;
    case 3: return TypePathKind::TYPE_ARGUMENT;
    default: throw std::invalid_argument("Invalid type path kind: " + std::to_string(value));
    }
}

}   // namespace jclassfile::model
