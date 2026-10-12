// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/ElementValueConstants.java
// 状态：阶段 6 完整实现
// 备注：原版是 interface + char 字段；C++ 用 struct + static constexpr char。

#pragma once

namespace jclassfile::model {

struct ElementValueConstants
{
    static constexpr char TAG_BYTE       = 'B';
    static constexpr char TAG_CHAR       = 'C';
    static constexpr char TAG_DOUBLE     = 'D';
    static constexpr char TAG_FLOAT      = 'F';
    static constexpr char TAG_INT        = 'I';
    static constexpr char TAG_LONG       = 'J';
    static constexpr char TAG_SHORT      = 'S';
    static constexpr char TAG_BOOLEAN    = 'Z';
    static constexpr char TAG_STRING     = 's';
    static constexpr char TAG_ENUM       = 'e';
    static constexpr char TAG_CLASS      = 'c';
    static constexpr char TAG_ANNOTATION = '@';
    static constexpr char TAG_ARRAY      = '[';
};

}   // namespace jclassfile::model
