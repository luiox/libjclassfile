// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/AttributeVersions.java
// 状态：阶段 3 实现
// 备注：原版 Java 17 switch 表达式 → C++17 if-else 链。逐 case 与原版一致。

#include "jclassfile/model/attribute/attribute_versions.hpp"

using namespace std::string_view_literals;

namespace jclassfile::model {

int AttributeVersions::getIntroducedVersion(std::string_view attributeName)
{
    if (attributeName == CODE || attributeName == CONSTANT_VALUE || attributeName == DEPRECATED ||
        attributeName == EXCEPTIONS || attributeName == INNER_CLASSES ||
        attributeName == LINE_NUMBER_TABLE || attributeName == LOCAL_VARIABLE_TABLE ||
        attributeName == SOURCE_FILE || attributeName == SYNTHETIC) {
        return JAVA1;
    }
    if (attributeName == ANNOTATION_DEFAULT || attributeName == ENCLOSING_METHOD ||
        attributeName == LOCAL_VARIABLE_TYPE_TABLE ||
        attributeName == RUNTIME_INVISIBLE_ANNOTATIONS ||
        attributeName == RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS ||
        attributeName == RUNTIME_VISIBLE_ANNOTATIONS ||
        attributeName == RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS || attributeName == SIGNATURE ||
        attributeName == SOURCE_DEBUG_EXTENSION) {
        return JAVA5;
    }
    if (attributeName == STACK_MAP_TABLE) {
        return JAVA6;
    }
    if (attributeName == CHARACTER_RANGE_TABLE || attributeName == COMPILATION_ID ||
        attributeName == SOURCE_ID || attributeName == BOOTSTRAP_METHODS) {
        return JAVA7;
    }
    if (attributeName == METHOD_PARAMETERS || attributeName == RUNTIME_INVISIBLE_TYPE_ANNOTATIONS ||
        attributeName == RUNTIME_VISIBLE_TYPE_ANNOTATIONS) {
        return JAVA8;
    }
    if (attributeName == MODULE || attributeName == MODULE_MAIN_CLASS ||
        attributeName == MODULE_PACKAGES || attributeName == MODULE_HASHES ||
        attributeName == MODULE_RESOLUTION || attributeName == MODULE_TARGET) {
        return JAVA9;
    }
    if (attributeName == NEST_HOST || attributeName == NEST_MEMBERS) {
        return JAVA11;
    }
    if (attributeName == RECORD) {
        return JAVA14;
    }
    if (attributeName == PERMITTED_SUBCLASSES) {
        return JAVA15;
    }
    return -1;
}

}   // namespace jclassfile::model
