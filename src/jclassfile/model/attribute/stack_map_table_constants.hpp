// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/StackMapTableConstants.java
// 状态：阶段 3 实现
// 备注：verification_type_info 的 tag 值与 stack map frame type 区间。与原版 1:1。

#pragma once

namespace jclassfile::model {

class StackMapTableConstants
{
public:
    static constexpr int ITEM_TOP                = 0;
    static constexpr int ITEM_INTEGER            = 1;
    static constexpr int ITEM_FLOAT              = 2;
    static constexpr int ITEM_DOUBLE             = 3;
    static constexpr int ITEM_LONG               = 4;
    static constexpr int ITEM_NULL               = 5;
    static constexpr int ITEM_UNINITIALIZED_THIS = 6;
    static constexpr int ITEM_OBJECT             = 7;
    static constexpr int ITEM_UNINITIALIZED      = 8;

    static constexpr int SAME_FRAME_MIN                          = 0;
    static constexpr int SAME_FRAME_MAX                          = 63;
    static constexpr int SAME_LOCALS_ONE_STACK_ITEM_MIN          = 64;
    static constexpr int SAME_LOCALS_ONE_STACK_ITEM_MAX          = 127;
    static constexpr int SAME_LOCALS_ONE_STACK_ITEM_EXTENDED_MIN = 247;
    static constexpr int SAME_LOCALS_ONE_STACK_ITEM_EXTENDED_MAX = 247;
    static constexpr int CHOP_FRAME_MIN                          = 248;
    static constexpr int CHOP_FRAME_MAX                          = 250;
    static constexpr int SAME_FRAME_EXTENDED_MIN                 = 251;
    static constexpr int SAME_FRAME_EXTENDED_MAX                 = 251;
    static constexpr int APPEND_FRAME_MIN                        = 252;
    static constexpr int APPEND_FRAME_MAX                        = 254;
    static constexpr int FULL_FRAME_MIN                          = 255;
    static constexpr int FULL_FRAME_MAX                          = 255;

    virtual ~StackMapTableConstants() = default;
};

}   // namespace jclassfile::model
