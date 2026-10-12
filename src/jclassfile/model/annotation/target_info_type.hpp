// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/TargetInfoType.java
// 状态：阶段 6 完整实现
// 备注：
//   - 9 个 enum value 一比一。
//   - fromTargetType(int) 静态方法按原版 switch 表达式分组。
//   - 项目禁用 RTTI：调用方通过 TargetInfo::kind() 枚举做类型分派，
//     不依赖 dynamic_cast / instanceof。

#pragma once

#include <stdexcept>
#include <string>

#include "jclassfile/model/annotation/annotation_constants.hpp"

namespace jclassfile::model {

enum class TargetInfoType
{
    TYPE_PARAMETER_TARGET,
    SUPERTYPE_TARGET,
    TYPE_PARAMETER_BOUND_TARGET,
    EMPTY_TARGET,
    FORMAL_PARAMETER_TARGET,
    THROWS_TARGET,
    LOCALVAR_TARGET,
    CATCH_TARGET,
    OFFSET_TARGET,
    TYPE_ARGUMENT_TARGET,
};

inline TargetInfoType targetInfoTypeFromTargetType(int type)
{
    switch (type) {
    case AnnotationConstants::PARAMETER_OF_CLASS_OR_INTERFACE:
    case AnnotationConstants::PARAMETER_OF_METHOD: return TargetInfoType::TYPE_PARAMETER_TARGET;
    case AnnotationConstants::SUPERTYPE: return TargetInfoType::SUPERTYPE_TARGET;
    case AnnotationConstants::BOUND_TYPE_PARAMETER_OF_CLASS:
    case AnnotationConstants::BOUND_TYPE_PARAMETER_OF_METHOD:
        return TargetInfoType::TYPE_PARAMETER_BOUND_TARGET;
    case AnnotationConstants::FIELD:
    case AnnotationConstants::METHOD_RETURN_TYPE:
    case AnnotationConstants::METHOD_RECEIVER_TYPE: return TargetInfoType::EMPTY_TARGET;
    case AnnotationConstants::METHOD_PARAMETER: return TargetInfoType::FORMAL_PARAMETER_TARGET;
    case AnnotationConstants::METHOD_THROWS: return TargetInfoType::THROWS_TARGET;
    case AnnotationConstants::LOCAL_VARIABLE_DECLARATION:
    case AnnotationConstants::RESOURCE_VARIABLE_DECLARATION: return TargetInfoType::LOCALVAR_TARGET;
    case AnnotationConstants::EXCEPTION_PARAMETER_DECLARATION: return TargetInfoType::CATCH_TARGET;
    case AnnotationConstants::INSTANCEOF_EXPRESSION:
    case AnnotationConstants::NEW_EXPRESSION:
    case AnnotationConstants::LAMBDA_NEW_EXPRESSION:
    case AnnotationConstants::LAMBDA_METHOD_REF_EXPRESSION: return TargetInfoType::OFFSET_TARGET;
    case AnnotationConstants::CAST_EXPRESSION:
    case AnnotationConstants::TYPE_ARGUMENT_OF_NEW_GENERIC_EXPRESSION:
    case AnnotationConstants::TYPE_ARGUMENT_OF_GENERIC_NEW_METHOD_REF_EXPRESSION:
    case AnnotationConstants::TYPE_ARGUMENT_OF_GENERIC_NEW_LAMBDA_CONSTRUCTOR_EXPRESSION:
    case AnnotationConstants::TYPE_ARGUMENT_OF_GENERIC_METHOD_REF_EXPRESSION:
        return TargetInfoType::TYPE_ARGUMENT_TARGET;
    default:
        throw std::invalid_argument("Invalid type annotation target_type value: " +
                                    std::to_string(type));
    }
}

}   // namespace jclassfile::model
