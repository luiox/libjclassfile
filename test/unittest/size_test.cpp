// 阶段 7：指令 size 计算测试
// 对齐 CAFED00D SizeTest

#include <gtest/gtest.h>

#include <memory>

#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/instruction/basic_instruction.hpp"
#include "jclassfile/model/instruction/cp_ref_instruction.hpp"
#include "jclassfile/model/instruction/iinc_instruction.hpp"
#include "jclassfile/model/instruction/int_operand_instruction.hpp"
#include "jclassfile/model/instruction/lookup_switch_instruction.hpp"
#include "jclassfile/model/instruction/multi_a_new_array_instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"
#include "jclassfile/model/instruction/table_switch_instruction.hpp"
#include "jclassfile/model/instruction/wide_instruction.hpp"

namespace {

using namespace jclassfile::model;
using namespace jclassfile::model::Opcodes;

TEST(InstructionSizeTest, BasicInstructions)
{
    EXPECT_EQ(BasicInstruction(NOP).computeSize(), 1);
    EXPECT_EQ(BasicInstruction(ICONST_0).computeSize(), 1);
    EXPECT_EQ(BasicInstruction(IADD).computeSize(), 1);
    EXPECT_EQ(BasicInstruction(RETURN).computeSize(), 1);
}

TEST(InstructionSizeTest, IntOperandInstructions)
{
    // 1 byte operand
    EXPECT_EQ(IntOperandInstruction(BIPUSH, 10).computeSize(), 2);
    EXPECT_EQ(IntOperandInstruction(RET, 5).computeSize(), 2);
    EXPECT_EQ(IntOperandInstruction(NEWARRAY, 10).computeSize(), 2);

    // 2 byte operand
    EXPECT_EQ(IntOperandInstruction(SIPUSH, 300).computeSize(), 3);
    EXPECT_EQ(IntOperandInstruction(IFEQ, 100).computeSize(), 3);
    EXPECT_EQ(IntOperandInstruction(IFNE, -50).computeSize(), 3);
    EXPECT_EQ(IntOperandInstruction(IF_ICMPEQ, 200).computeSize(), 3);
    EXPECT_EQ(IntOperandInstruction(GOTO, -10).computeSize(), 3);
    EXPECT_EQ(IntOperandInstruction(JSR, 50).computeSize(), 3);
    EXPECT_EQ(IntOperandInstruction(IFNULL, 25).computeSize(), 3);
    EXPECT_EQ(IntOperandInstruction(IFNONNULL, -30).computeSize(), 3);

    // 4 byte operand
    EXPECT_EQ(IntOperandInstruction(GOTO_W, 10000).computeSize(), 5);
    EXPECT_EQ(IntOperandInstruction(JSR_W, -5000).computeSize(), 5);

    // xload/xstore single byte
    EXPECT_EQ(IntOperandInstruction(ILOAD, 3).computeSize(), 2);
    EXPECT_EQ(IntOperandInstruction(ISTORE, 3).computeSize(), 2);
    EXPECT_EQ(IntOperandInstruction(ALOAD, 1).computeSize(), 2);
    EXPECT_EQ(IntOperandInstruction(ASTORE, 2).computeSize(), 2);
    EXPECT_EQ(IntOperandInstruction(FLOAD, 0).computeSize(), 2);
    EXPECT_EQ(IntOperandInstruction(DLOAD, 1).computeSize(), 2);
    EXPECT_EQ(IntOperandInstruction(LLOAD, 3).computeSize(), 2);
    EXPECT_EQ(IntOperandInstruction(LSTORE, 2).computeSize(), 2);
}

TEST(InstructionSizeTest, CpRefInstructions)
{
    auto utf8  = std::make_unique<CpUtf8>("Owner");
    auto klass = std::make_unique<CpClass>(utf8.get());

    EXPECT_EQ(CpRefInstruction(LDC, klass.get()).computeSize(), 2);
    EXPECT_EQ(CpRefInstruction(GETSTATIC, klass.get()).computeSize(), 3);
    EXPECT_EQ(CpRefInstruction(INVOKEINTERFACE, klass.get()).computeSize(), 5);
    EXPECT_EQ(CpRefInstruction(INVOKEDYNAMIC, klass.get()).computeSize(), 5);
}

TEST(InstructionSizeTest, IincInstruction)
{
    // Normal: 3 bytes
    EXPECT_EQ(IincInstruction(5, -3).computeSize(), 3);
    EXPECT_EQ(IincInstruction(0, 0).computeSize(), 3);
    EXPECT_EQ(IincInstruction(255, 127).computeSize(), 3);
}

TEST(InstructionSizeTest, WideInstruction)
{
    // WIDE + non-IINC backing: 4 bytes
    EXPECT_EQ(WideInstruction(std::make_unique<IntOperandInstruction>(ILOAD, 300)).computeSize(),
              4);
    EXPECT_EQ(WideInstruction(std::make_unique<IntOperandInstruction>(ISTORE, 300)).computeSize(),
              4);
    EXPECT_EQ(WideInstruction(std::make_unique<IntOperandInstruction>(ALOAD, 300)).computeSize(),
              4);
    EXPECT_EQ(WideInstruction(std::make_unique<IntOperandInstruction>(RET, 300)).computeSize(), 4);

    // WIDE + IINC backing: 6 bytes
    EXPECT_EQ(WideInstruction(std::make_unique<IincInstruction>(300, 1000)).computeSize(), 6);
}

TEST(InstructionSizeTest, MultiANewArrayInstruction)
{
    auto utf8  = std::make_unique<CpUtf8>("[[I");
    auto klass = std::make_unique<CpClass>(utf8.get());
    EXPECT_EQ(MultiANewArrayInstruction(klass.get(), 2).computeSize(), 4);
}

TEST(InstructionSizeTest, SwitchInstructions)
{
    // TABLESWITCH and LOOKUPSWITCH size depends on padding
    // Without position notification, computeSize returns -1 (unknown)
    TableSwitchInstruction ts(0, 0, 2, {0, 10, 20});
    EXPECT_EQ(ts.computeSize(), -1);   // Not notified

    ts.notifyStartPosition(0);   // At position 0, padding = 3 bytes
    // opcode(1) + padding(3) + default(4) + low(4) + high(4) + 3 offsets(12) = 28
    EXPECT_EQ(ts.computeSize(), 28);

    // LOOKUPSWITCH
    LookupSwitchInstruction ls(0, {0, 10}, {20, 30});
    EXPECT_EQ(ls.computeSize(), -1);

    ls.notifyStartPosition(0);
    // opcode(1) + padding(3) + default(4) + npairs(4) + 2*(key+offset)(16) = 28
    EXPECT_EQ(ls.computeSize(), 28);
}

TEST(InstructionSizeTest, SwitchWithDifferentAlignment)
{
    // Test at different start positions
    TableSwitchInstruction ts(0, 0, 1, {0, 5});
    ts.notifyStartPosition(1);   // position 1, padding = 2
    // 1 + 2 + 4 + 4 + 4 + 8 = 23
    EXPECT_EQ(ts.computeSize(), 23);

    ts.notifyStartPosition(2);   // position 2, padding = 1
    EXPECT_EQ(ts.computeSize(), 22);

    ts.notifyStartPosition(3);   // position 3, padding = 0
    EXPECT_EQ(ts.computeSize(), 21);

    ts.notifyStartPosition(4);   // position 4, padding = 3
    EXPECT_EQ(ts.computeSize(), 24);
}

}   // namespace
