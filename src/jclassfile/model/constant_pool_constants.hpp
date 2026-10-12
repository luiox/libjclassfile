// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/ConstantPoolConstants.java
// 状态：阶段 2 实现
// 备注：JVM spec §4.4 The Constant Pool。Tag 值固定（CpEntry.getTag() 用于分发）。
//       wide 指 LONG(5)/DOUBLE(6)，占两个 slot。

#pragma once

#include "libca/core/datatype.hpp"

namespace jclassfile::model {

enum class CpTag : ca::i8
{
    INTERNAL             = 0,
    UTF8                 = 1,
    INTEGER              = 3,
    FLOAT                = 4,
    LONG                 = 5,
    DOUBLE               = 6,
    CLASS                = 7,
    STRING               = 8,
    FIELD_REF            = 9,
    METHOD_REF           = 10,
    INTERFACE_METHOD_REF = 11,
    NAME_TYPE            = 12,
    METHOD_HANDLE        = 15,
    METHOD_TYPE          = 16,
    DYNAMIC              = 17,
    INVOKE_DYNAMIC       = 18,
    MODULE               = 19,
    PACKAGE              = 20,
};

}   // namespace jclassfile::model
