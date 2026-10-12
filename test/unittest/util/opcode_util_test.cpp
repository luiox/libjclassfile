// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/util/OpcodeUtil.java
// 覆盖：
//   - Opcodes 字段（0..201）按声明顺序生成 names[opcode] = lowercased name
//   - ReservedOpcodes 字段（202..241）按声明顺序追加
//   - 重复值（shouldnotreachhere_v8/v9/v11）按"反射顺序追加"行为填入 239/240/241
//   - 越界（< 0, >= 256）返回 nullptr

#include <gtest/gtest.h>

#include "jclassfile/util/opcode_util.hpp"

using jclassfile::util::OpcodeUtil;

TEST(OpcodeUtilTest, NopIsZero)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(0), "nop");
}

TEST(OpcodeUtilTest, AconstNullIsOne)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(1), "aconst_null");
}

TEST(OpcodeUtilTest, IconstM1IsTwo)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(2), "iconst_m1");
}

TEST(OpcodeUtilTest, IconstRange)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(3), "iconst_0");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(4), "iconst_1");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(5), "iconst_2");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(6), "iconst_3");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(7), "iconst_4");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(8), "iconst_5");
}

TEST(OpcodeUtilTest, LoadRangeBoundaries)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(21), "iload");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(25), "aload");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(26), "iload_0");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(45), "aload_3");
}

TEST(OpcodeUtilTest, StoreRangeBoundaries)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(54), "istore");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(58), "astore");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(59), "istore_0");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(78), "astore_3");
}

TEST(OpcodeUtilTest, AaloadThroughSaload)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(50), "aaload");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(51), "baload");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(52), "caload");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(53), "saload");
}

TEST(OpcodeUtilTest, StackManipulation)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(87), "pop");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(88), "pop2");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(89), "dup");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(90), "dup_x1");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(91), "dup_x2");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(92), "dup2");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(95), "swap");
}

TEST(OpcodeUtilTest, ArithRange)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(96), "iadd");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(99), "dadd");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(100), "isub");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(104), "imul");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(108), "idiv");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(112), "irem");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(116), "ineg");
}

TEST(OpcodeUtilTest, Bitwise)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(120), "ishl");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(124), "iushr");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(126), "iand");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(128), "ior");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(130), "ixor");
}

TEST(OpcodeUtilTest, IincIs132)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(132), "iinc");
}

TEST(OpcodeUtilTest, Conversions)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(133), "i2l");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(138), "l2d");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(145), "i2b");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(147), "i2s");
}

TEST(OpcodeUtilTest, Comparisons)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(148), "lcmp");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(153), "ifeq");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(159), "if_icmpeq");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(165), "if_acmpeq");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(198), "ifnull");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(199), "ifnonnull");
}

TEST(OpcodeUtilTest, ControlFlow)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(167), "goto");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(168), "jsr");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(169), "ret");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(170), "tableswitch");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(171), "lookupswitch");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(177), "return");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(200), "goto_w");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(201), "jsr_w");
}

TEST(OpcodeUtilTest, FieldAndInvoke)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(178), "getstatic");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(180), "getfield");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(182), "invokevirtual");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(185), "invokeinterface");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(186), "invokedynamic");
}

TEST(OpcodeUtilTest, ObjectArray)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(187), "new");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(188), "newarray");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(189), "anewarray");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(190), "arraylength");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(191), "athrow");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(192), "checkcast");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(193), "instanceof");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(194), "monitorenter");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(195), "monitorexit");
}

TEST(OpcodeUtilTest, WideAndMultianewarray)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(196), "wide");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(197), "multianewarray");
}

TEST(OpcodeUtilTest, ReservedBreakpointIs202)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(202), "breakpoint");
}

TEST(OpcodeUtilTest, ReservedFastAgetfield)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(203), "fast_agetfield");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(210), "fast_sgetfield");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(220), "fast_aload_0");
}

TEST(OpcodeUtilTest, ReservedInvokehandle)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(233), "invokehandle");
}

TEST(OpcodeUtilTest, ReservedShouldNotReachHere)
{
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(238), "shouldnotreachhere");
}

TEST(OpcodeUtilTest, VersionedShouldNotReachHereFillTrailingSlots)
{
    // 反射顺序下，v8/v9/v11 三个常量按声明追加到 239/240/241
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(239), "shouldnotreachhere_v8");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(240), "shouldnotreachhere_v9");
    EXPECT_STREQ(OpcodeUtil::getOpcodeName(241), "shouldnotreachhere_v11");
}

TEST(OpcodeUtilTest, TrailingSlotsAreNull)
{
    EXPECT_EQ(OpcodeUtil::getOpcodeName(242), nullptr);
    EXPECT_EQ(OpcodeUtil::getOpcodeName(255), nullptr);
}

TEST(OpcodeUtilTest, NegativeReturnsNull)
{
    EXPECT_EQ(OpcodeUtil::getOpcodeName(-1), nullptr);
    EXPECT_EQ(OpcodeUtil::getOpcodeName(-100), nullptr);
}

TEST(OpcodeUtilTest, OutOfRangeReturnsNull)
{
    EXPECT_EQ(OpcodeUtil::getOpcodeName(256), nullptr);
    EXPECT_EQ(OpcodeUtil::getOpcodeName(1000), nullptr);
}
