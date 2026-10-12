// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/AttributeConstants.java
// 状态：阶段 3 实现
// 备注：attribute 名字符串常量（与原版 interface 中 String 常量完全一致）。

#pragma once

#include <string_view>

namespace jclassfile::model {

class AttributeConstants
{
public:
    static constexpr std::string_view ANNOTATION_DEFAULT          = "AnnotationDefault";
    static constexpr std::string_view BOOTSTRAP_METHODS           = "BootstrapMethods";
    static constexpr std::string_view CHARACTER_RANGE_TABLE       = "CharacterRangeTable";
    static constexpr std::string_view CODE                        = "Code";
    static constexpr std::string_view CONSTANT_VALUE              = "ConstantValue";
    static constexpr std::string_view COMPILATION_ID              = "CompilationID";
    static constexpr std::string_view DEPRECATED                  = "Deprecated";
    static constexpr std::string_view ENCLOSING_METHOD            = "EnclosingMethod";
    static constexpr std::string_view EXCEPTIONS                  = "Exceptions";
    static constexpr std::string_view INNER_CLASSES               = "InnerClasses";
    static constexpr std::string_view LINE_NUMBER_TABLE           = "LineNumberTable";
    static constexpr std::string_view LOCAL_VARIABLE_TABLE        = "LocalVariableTable";
    static constexpr std::string_view LOCAL_VARIABLE_TYPE_TABLE   = "LocalVariableTypeTable";
    static constexpr std::string_view METHOD_PARAMETERS           = "MethodParameters";
    static constexpr std::string_view MODULE                      = "Module";
    static constexpr std::string_view MODULE_HASHES               = "ModuleHashes";
    static constexpr std::string_view MODULE_MAIN_CLASS           = "ModuleMainClass";
    static constexpr std::string_view MODULE_PACKAGES             = "ModulePackages";
    static constexpr std::string_view MODULE_RESOLUTION           = "ModuleResolution";
    static constexpr std::string_view MODULE_TARGET               = "ModuleTarget";
    static constexpr std::string_view NEST_HOST                   = "NestHost";
    static constexpr std::string_view NEST_MEMBERS                = "NestMembers";
    static constexpr std::string_view RECORD                      = "Record";
    static constexpr std::string_view RUNTIME_VISIBLE_ANNOTATIONS = "RuntimeVisibleAnnotations";
    static constexpr std::string_view RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS =
        "RuntimeVisibleParameterAnnotations";
    static constexpr std::string_view RUNTIME_VISIBLE_TYPE_ANNOTATIONS =
        "RuntimeVisibleTypeAnnotations";
    static constexpr std::string_view RUNTIME_INVISIBLE_ANNOTATIONS = "RuntimeInvisibleAnnotations";
    static constexpr std::string_view RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS =
        "RuntimeInvisibleParameterAnnotations";
    static constexpr std::string_view RUNTIME_INVISIBLE_TYPE_ANNOTATIONS =
        "RuntimeInvisibleTypeAnnotations";
    static constexpr std::string_view PERMITTED_SUBCLASSES   = "PermittedSubclasses";
    static constexpr std::string_view SIGNATURE              = "Signature";
    static constexpr std::string_view SOURCE_DEBUG_EXTENSION = "SourceDebugExtension";
    static constexpr std::string_view SOURCE_FILE            = "SourceFile";
    static constexpr std::string_view SOURCE_ID              = "SourceID";
    static constexpr std::string_view STACK_MAP_TABLE        = "StackMapTable";
    static constexpr std::string_view SYNTHETIC              = "Synthetic";

    virtual ~AttributeConstants() = default;
};

}   // namespace jclassfile::model
