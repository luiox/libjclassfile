// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/ReservedOpcodes.java
// 状态：阶段 5a 实现（header-only 命名空间常量）
// 备注：Java `interface ReservedOpcodes` 字段；C++ 等价于 namespace + inline constexpr。
//       含三个版本相关常量（shouldnotreachhere_v8/v9/v11）值与主条目重复：v8=233=invokehandle,
//       v9=234=nofast_getfield, v11=238=shouldnotreachhere。OpcodeUtil.cpp 按反射顺序填表
//       体现这一行为。

#pragma once

namespace jclassfile::model::ReservedOpcodes {

inline constexpr int breakpoint                = 202;
inline constexpr int fast_agetfield            = 203;
inline constexpr int fast_bgetfield            = 204;
inline constexpr int fast_cgetfield            = 205;
inline constexpr int fast_dgetfield            = 206;
inline constexpr int fast_fgetfield            = 207;
inline constexpr int fast_igetfield            = 208;
inline constexpr int fast_lgetfield            = 209;
inline constexpr int fast_sgetfield            = 210;
inline constexpr int fast_aputfield            = 211;
inline constexpr int fast_bputfield            = 212;
inline constexpr int fast_zputfield            = 213;
inline constexpr int fast_cputfield            = 214;
inline constexpr int fast_dputfield            = 215;
inline constexpr int fast_fputfield            = 216;
inline constexpr int fast_iputfield            = 217;
inline constexpr int fast_lputfield            = 218;
inline constexpr int fast_sputfield            = 219;
inline constexpr int fast_aload_0              = 220;
inline constexpr int fast_iaccess_0            = 221;
inline constexpr int fast_aaccess_0            = 222;
inline constexpr int fast_faccess_0            = 223;
inline constexpr int fast_iload                = 224;
inline constexpr int fast_iload2               = 225;
inline constexpr int fast_icaload              = 226;
inline constexpr int fast_invokevfinal         = 227;
inline constexpr int fast_linearswitch         = 228;
inline constexpr int fast_binaryswitch         = 229;
inline constexpr int fast_aldc                 = 230;
inline constexpr int fast_aldc_w               = 231;
inline constexpr int return_register_finalizer = 232;
inline constexpr int invokehandle              = 233;
inline constexpr int nofast_getfield           = 234;
inline constexpr int nofast_putfield           = 235;
inline constexpr int nofast_aload_0            = 236;
inline constexpr int nofast_iload              = 237;
inline constexpr int shouldnotreachhere        = 238;
inline constexpr int shouldnotreachhere_v8     = 233;
inline constexpr int shouldnotreachhere_v9     = 234;
inline constexpr int shouldnotreachhere_v11    = 238;

}   // namespace jclassfile::model::ReservedOpcodes
