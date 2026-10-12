// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/AttributeContexts.java
// 状态：阶段 3 实现
// 备注：原版 switch 表达式按 attribute 名字符串返回允许的 AttributeHolderType 集合；
//       C++ 用 if-else if 链实现，default 行为 = 全位置允许。

#include "jclassfile/model/attribute/attribute_contexts.hpp"

using namespace jclassfile::io;

namespace jclassfile::model {

std::vector<AttributeHolderType> AttributeContexts::enumAll()
{
    return {AttributeHolderType::CLASS,
            AttributeHolderType::FIELD,
            AttributeHolderType::METHOD,
            AttributeHolderType::RECORD_COMPONENT,
            AttributeHolderType::ATTRIBUTE};
}

std::vector<AttributeHolderType> AttributeContexts::enumClass()
{
    return {AttributeHolderType::CLASS};
}

std::vector<AttributeHolderType> AttributeContexts::enumField()
{
    return {AttributeHolderType::FIELD};
}

std::vector<AttributeHolderType> AttributeContexts::enumMethod()
{
    return {AttributeHolderType::METHOD};
}

std::vector<AttributeHolderType> AttributeContexts::enumClassFieldMethod()
{
    return {AttributeHolderType::CLASS, AttributeHolderType::FIELD, AttributeHolderType::METHOD};
}

std::vector<AttributeHolderType> AttributeContexts::enumAttribute()
{
    return {AttributeHolderType::ATTRIBUTE};
}

/// CLASS/FIELD/METHOD/RECORD_COMPONENT 四上下文（不含 ATTRIBUTE 宿主）。
/// 对应上游 EnumSet.of(CLASS, FIELD, METHOD, RECORD_COMPONENT)。
static std::vector<AttributeHolderType> enumClassFieldMethodRecord()
{
    return {AttributeHolderType::CLASS, AttributeHolderType::FIELD, AttributeHolderType::METHOD,
            AttributeHolderType::RECORD_COMPONENT};
}

std::vector<AttributeHolderType> AttributeContexts::getAllowedContexts(
    std::string_view attributeName)
{
    using namespace std::string_view_literals;
    if (attributeName == BOOTSTRAP_METHODS || attributeName == COMPILATION_ID ||
        attributeName == ENCLOSING_METHOD || attributeName == INNER_CLASSES ||
        attributeName == MODULE || attributeName == MODULE_HASHES ||
        attributeName == MODULE_MAIN_CLASS || attributeName == MODULE_PACKAGES ||
        attributeName == MODULE_RESOLUTION || attributeName == MODULE_TARGET ||
        attributeName == NEST_HOST || attributeName == NEST_MEMBERS ||
        attributeName == PERMITTED_SUBCLASSES || attributeName == RECORD ||
        attributeName == SOURCE_DEBUG_EXTENSION || attributeName == SOURCE_FILE ||
        attributeName == SOURCE_ID) {
        return enumClass();
    }
    if (attributeName == CONSTANT_VALUE) {
        return enumField();
    }
    if (attributeName == ANNOTATION_DEFAULT || attributeName == CODE ||
        attributeName == EXCEPTIONS || attributeName == METHOD_PARAMETERS ||
        attributeName == RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS ||
        attributeName == RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS) {
        return enumMethod();
    }
    if (attributeName == DEPRECATED || attributeName == SYNTHETIC) {
        return enumClassFieldMethod();
    }
    if (attributeName == LINE_NUMBER_TABLE || attributeName == LOCAL_VARIABLE_TABLE ||
        attributeName == LOCAL_VARIABLE_TYPE_TABLE || attributeName == STACK_MAP_TABLE ||
        attributeName == CHARACTER_RANGE_TABLE) {
        return enumAttribute();
    }
    // 类型注解可挂全部宿主（含 Code/属性内嵌，对应上游 EnumSet.allOf）。
    if (attributeName == RUNTIME_VISIBLE_TYPE_ANNOTATIONS ||
        attributeName == RUNTIME_INVISIBLE_TYPE_ANNOTATIONS) {
        return enumAll();
    }
    // 普通注解与 Signature 只挂四成员上下文，不进 ATTRIBUTE 宿主
    // （上游 aadd6fcf：与类型注解拆分）。
    if (attributeName == RUNTIME_VISIBLE_ANNOTATIONS ||
        attributeName == RUNTIME_INVISIBLE_ANNOTATIONS || attributeName == SIGNATURE) {
        return enumClassFieldMethodRecord();
    }
    // default: unknown attribute, allow anywhere
    return enumAll();
}

}   // namespace jclassfile::model
