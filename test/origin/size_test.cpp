// CAFED00D 1:1 mirror: SizeTest.java
// Tests for instruction size computation.

#include <gtest/gtest.h>

#include <vector>

#include "jclassfile/model/instruction/basic_instruction.hpp"
#include "jclassfile/model/instruction/cp_ref_instruction.hpp"
#include "jclassfile/model/instruction/iinc_instruction.hpp"
#include "jclassfile/model/instruction/int_operand_instruction.hpp"
#include "jclassfile/model/instruction/lookup_switch_instruction.hpp"
#include "jclassfile/model/instruction/multi_a_new_array_instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"
#include "jclassfile/model/instruction/table_switch_instruction.hpp"
#include "jclassfile/model/instruction/wide_instruction.hpp"
#include "jclassfile/model/constant/placeholders.hpp"

namespace {

using namespace jclassfile::model;
using namespace jclassfile::model::Opcodes;

TEST(SizeTest_Instruction, BasicInstruction)
{
    BasicInstruction instruction(NOP);
    EXPECT_EQ(instruction.computeSize(), 1);
}

TEST(SizeTest_Instruction, MultiANewArrayInstruction)
{
    // Size is 4 bytes (opcode + cp_index + dims)
    MultiANewArrayInstruction instruction(Placeholders::klass(), 0);
    EXPECT_EQ(instruction.computeSize(), 4);
}

TEST(SizeTest_Instruction, IincInstruction)
{
    // Size is 3 bytes (opcode + operand_var + operand_incr)
    IincInstruction instruction(1, 1);
    EXPECT_EQ(instruction.computeSize(), 3);
}

TEST(SizeTest_Instruction, WideInstruction)
{
    // IINC wide is 6 bytes
    auto instruction = WideInstruction(std::make_unique<IincInstruction>(0, 0));
    EXPECT_EQ(instruction.computeSize(), 6);

    int fourByteOps[] = {
        LLOAD, DLOAD, LSTORE, DSTORE, RET, ILOAD, FLOAD, ALOAD, ISTORE, FSTORE, ASTORE};
    for (int op : fourByteOps) {
        WideInstruction wide_instruction(std::make_unique<IntOperandInstruction>(op, 0));
        EXPECT_EQ(wide_instruction.computeSize(), 4);
    }
}

// CAFED00D SizeTest.Instruction.testCpRefInstruction
TEST(SizeTest_Instruction, CpRefInstruction)
{
    int threeByteOps[] = {LDC_W,
                          LDC2_W,
                          GETSTATIC,
                          PUTSTATIC,
                          GETFIELD,
                          PUTFIELD,
                          INVOKEVIRTUAL,
                          INVOKESPECIAL,
                          INVOKESTATIC,
                          NEW,
                          ANEWARRAY,
                          CHECKCAST,
                          INSTANCEOF};
    for (int op : threeByteOps) {
        CpRefInstruction instruction(op, Placeholders::constRef());
        EXPECT_EQ(instruction.computeSize(), 3);
    }
    // LDC = 2 bytes
    EXPECT_EQ(CpRefInstruction(LDC, Placeholders::constRef()).computeSize(), 2);
    // INVOKEDYNAMIC = 5 bytes
    EXPECT_EQ(CpRefInstruction(INVOKEDYNAMIC, Placeholders::constRef()).computeSize(), 5);
    // INVOKEINTERFACE = 5 bytes
    EXPECT_EQ(CpRefInstruction(INVOKEINTERFACE, Placeholders::constRef()).computeSize(), 5);
}

// CAFED00D SizeTest.Instruction.testIntOperandInstruction
TEST(SizeTest_Instruction, IntOperandInstruction)
{
    int twoByteOps[] = {RET,
                        BIPUSH,
                        ALOAD,
                        ASTORE,
                        DLOAD,
                        DSTORE,
                        FLOAD,
                        FSTORE,
                        ILOAD,
                        ISTORE,
                        LLOAD,
                        LSTORE,
                        NEWARRAY};
    for (int op : twoByteOps) {
        EXPECT_EQ(IntOperandInstruction(op, 0).computeSize(), 2);
    }

    int threeByteOps[] = {SIPUSH,
                          IFEQ,
                          IFNE,
                          IFLT,
                          IFGE,
                          IFGT,
                          IFLE,
                          IF_ICMPEQ,
                          IF_ICMPNE,
                          IF_ICMPLT,
                          IF_ICMPGE,
                          IF_ICMPGT,
                          IF_ICMPLE,
                          IF_ACMPEQ,
                          IF_ACMPNE,
                          IFNULL,
                          IFNONNULL,
                          GOTO,
                          JSR};
    for (int op : threeByteOps) {
        EXPECT_EQ(IntOperandInstruction(op, 0).computeSize(), 3);
    }
}

// CAFED00D SizeTest.Instruction.testLookupSwitchInstruction
TEST(SizeTest_Instruction, LookupSwitchInstruction)
{
    {
        LookupSwitchInstruction insn(0, {}, {});
        insn.notifyStartPosition(0);
        EXPECT_EQ(insn.computeSize(), 12);   // 1 + 3 + 4 + 4 = 12
        insn.notifyStartPosition(1);
        EXPECT_EQ(insn.computeSize(), 11);   // 1 + 2 + 4 + 4 = 11
        insn.notifyStartPosition(2);
        EXPECT_EQ(insn.computeSize(), 10);   // 1 + 1 + 4 + 4 = 10
        insn.notifyStartPosition(3);
        EXPECT_EQ(insn.computeSize(), 9);   // 1 + 0 + 4 + 4 = 9
    }
    // One pair: +8 bytes
    {
        LookupSwitchInstruction insn(0, {1}, {1});
        insn.notifyStartPosition(3);
        EXPECT_EQ(insn.computeSize(), 17);   // 9 + 8 = 17
    }
}

// CAFED00D SizeTest.Instruction.testTableSwitchInstruction
TEST(SizeTest_Instruction, TableSwitchInstruction)
{
    {
        TableSwitchInstruction insn(0, 0, 0, {});
        insn.notifyStartPosition(0);
        EXPECT_EQ(insn.computeSize(), 16);   // 1 + 3 + 4 + 4 + 4 = 16
        insn.notifyStartPosition(1);
        EXPECT_EQ(insn.computeSize(), 15);   // 1 + 2 + 4 + 4 + 4 = 15
        insn.notifyStartPosition(2);
        EXPECT_EQ(insn.computeSize(), 14);   // 1 + 1 + 4 + 4 + 4 = 14
        insn.notifyStartPosition(3);
        EXPECT_EQ(insn.computeSize(), 13);   // 1 + 0 + 4 + 4 + 4 = 13
    }
    // One entry: +4 bytes
    {
        TableSwitchInstruction insn(0, 0, 0, {1});
        insn.notifyStartPosition(3);
        EXPECT_EQ(insn.computeSize(), 17);   // 13 + 4 = 17
    }
}

}   // namespace

// TODO: Size tests for comparing against instruction write-back using InstructionWriter
// TODO: Size tests for other size-bound elements (e.g. Attribute)
