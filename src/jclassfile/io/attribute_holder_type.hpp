// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/AttributeHolderType.java
// 状态：阶段 1 + 阶段 6 完整实现
// 备注：5 个 enum 值一比一。
//       阶段 6 补全 fromAnnotationTargetType(int) 静态方法（原版阶段 6 才补全，
//       因为依赖 model/annotation/AnnotationConstants）。
//       上游 6489374e：映射对未知 target 不再抛异常（返回 nullopt），并新增
//       isAnnotationTargetTypeValidIn 校验（0x13 FIELD 目标允许 RECORD_COMPONENT
//       holder，修复 record component 上的注解被整属性丢弃）。

#pragma once

#include <optional>

#include "libca/core/datatype.hpp"
#include "jclassfile/model/annotation/annotation_constants.hpp"

namespace jclassfile::io {

enum class AttributeHolderType : ca::u8
{
    CLASS            = 0,
    FIELD            = 1,
    METHOD           = 2,
    RECORD_COMPONENT = 3,
    ATTRIBUTE        = 4,
};

// target_type → holder 映射；未知 target 返回 nullopt（原版 6489374e 起
// 返回 null 并记 debug 日志，不再抛异常）。
inline std::optional<AttributeHolderType> attributeHolderTypeFromAnnotationTargetType(int type)
{
    using namespace jclassfile::model;
    switch (type) {
    case AnnotationConstants::PARAMETER_OF_CLASS_OR_INTERFACE:
    case AnnotationConstants::BOUND_TYPE_PARAMETER_OF_CLASS:
    case AnnotationConstants::SUPERTYPE: return AttributeHolderType::CLASS;
    case AnnotationConstants::FIELD: return AttributeHolderType::FIELD;
    case AnnotationConstants::PARAMETER_OF_METHOD:
    case AnnotationConstants::BOUND_TYPE_PARAMETER_OF_METHOD:
    case AnnotationConstants::METHOD_RETURN_TYPE:
    case AnnotationConstants::METHOD_RECEIVER_TYPE:
    case AnnotationConstants::METHOD_PARAMETER:
    case AnnotationConstants::METHOD_THROWS: return AttributeHolderType::METHOD;
    case AnnotationConstants::LOCAL_VARIABLE_DECLARATION:
    case AnnotationConstants::RESOURCE_VARIABLE_DECLARATION:
    case AnnotationConstants::EXCEPTION_PARAMETER_DECLARATION:
    case AnnotationConstants::INSTANCEOF_EXPRESSION:
    case AnnotationConstants::NEW_EXPRESSION:
    case AnnotationConstants::LAMBDA_NEW_EXPRESSION:
    case AnnotationConstants::LAMBDA_METHOD_REF_EXPRESSION:
    case AnnotationConstants::CAST_EXPRESSION:
    case AnnotationConstants::TYPE_ARGUMENT_OF_NEW_GENERIC_EXPRESSION:
    case AnnotationConstants::TYPE_ARGUMENT_OF_GENERIC_NEW_METHOD_REF_EXPRESSION:
    case AnnotationConstants::TYPE_ARGUMENT_OF_GENERIC_NEW_LAMBDA_CONSTRUCTOR_EXPRESSION:
    case AnnotationConstants::TYPE_ARGUMENT_OF_GENERIC_METHOD_REF_EXPRESSION:
        return AttributeHolderType::ATTRIBUTE;
    default: return std::nullopt;
    }
}

// target_type 是否允许出现在给定 holder 中（原版 6489374e 新增）。
inline bool isAnnotationTargetTypeValidIn(int targetType, AttributeHolderType holderType)
{
    using namespace jclassfile::model;
    // target 0x13（FIELD / empty_target）同时用于 field 与 record component 声明
    // （JVMS 4.7.20.1），是唯一允许出现在两种 holder 的特例。
    if (targetType == AnnotationConstants::FIELD &&
        holderType == AttributeHolderType::RECORD_COMPONENT)
    {
        return true;
    }
    auto expected = attributeHolderTypeFromAnnotationTargetType(targetType);
    return expected != std::nullopt && expected == holderType;
}

}   // namespace jclassfile::io
