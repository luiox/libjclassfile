// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/AnnotationConstants.java
// 状态：阶段 6 完整实现
// 备注：target_type 字节常量（与原版完全一致）。原版用 interface + int 字段；
//       C++ 改用 struct + static constexpr 字段，避开 virtual destructor 虚函数表。
//       详细含义见原版 javadoc（TypeParameter / SuperType / Bound / Field /
//       MethodReturn / MethodReceiver / FormalParameter / Throws / LocalVar /
//       Catch / Instanceof / New / LambdaNew / LambdaMethodRef / Cast / TypeArgument*）。

#pragma once

namespace jclassfile::model {

struct AnnotationConstants
{
    static constexpr int PARAMETER_OF_CLASS_OR_INTERFACE                            = 0x00;
    static constexpr int PARAMETER_OF_METHOD                                        = 0x01;
    static constexpr int SUPERTYPE                                                  = 0x10;
    static constexpr int BOUND_TYPE_PARAMETER_OF_CLASS                              = 0x11;
    static constexpr int BOUND_TYPE_PARAMETER_OF_METHOD                             = 0x12;
    static constexpr int FIELD                                                      = 0x13;
    static constexpr int METHOD_RETURN_TYPE                                         = 0x14;
    static constexpr int METHOD_RECEIVER_TYPE                                       = 0x15;
    static constexpr int METHOD_PARAMETER                                           = 0x16;
    static constexpr int METHOD_THROWS                                              = 0x17;
    static constexpr int LOCAL_VARIABLE_DECLARATION                                 = 0x40;
    static constexpr int RESOURCE_VARIABLE_DECLARATION                              = 0x41;
    static constexpr int EXCEPTION_PARAMETER_DECLARATION                            = 0x42;
    static constexpr int INSTANCEOF_EXPRESSION                                      = 0x43;
    static constexpr int NEW_EXPRESSION                                             = 0x44;
    static constexpr int LAMBDA_NEW_EXPRESSION                                      = 0x45;
    static constexpr int LAMBDA_METHOD_REF_EXPRESSION                               = 0x46;
    static constexpr int CAST_EXPRESSION                                            = 0x47;
    static constexpr int TYPE_ARGUMENT_OF_NEW_GENERIC_EXPRESSION                    = 0x48;
    static constexpr int TYPE_ARGUMENT_OF_GENERIC_NEW_METHOD_REF_EXPRESSION         = 0x49;
    static constexpr int TYPE_ARGUMENT_OF_GENERIC_NEW_LAMBDA_CONSTRUCTOR_EXPRESSION = 0x4A;
    static constexpr int TYPE_ARGUMENT_OF_GENERIC_METHOD_REF_EXPRESSION             = 0x4B;
};

}   // namespace jclassfile::model
