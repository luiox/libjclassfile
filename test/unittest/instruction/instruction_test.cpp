// 阶段 5b：Instruction 8 个子类的单元测试
// 覆盖：
//   - 各子类的 getOpcode / computeSize / kind / equals / 字段 getter
//   - InsnKind 枚举分派（与 Java instanceof 行为等价）
//   - cpAccesses 默认空 + CpRef/MultiANewArray 覆盖 + Wide 委托
//   - TABLESWITCH/LOOKUPSWITCH padding 公式 3 - (position & 3)
//   - IntOperandInstruction computeSize 三档分派（1 字节/2 字节/4 字节）
//   - CpRefInstruction computeSize 三档分派（LDC=2 / INVOKE*=5 / 其他=3）

#include <gtest/gtest.h>

#include <memory>
#include <utility>
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
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

using namespace jclassfile::model;
using namespace jclassfile::model::Opcodes;

// ============== BasicInstruction ==============

TEST(BasicInstructionTest, KindAndSize)
{
    BasicInstruction nop(NOP);
    EXPECT_EQ(nop.kind(), InsnKind::Basic);
    EXPECT_EQ(nop.getOpcode(), NOP);
    EXPECT_EQ(nop.computeSize(), 1);
    EXPECT_EQ(nop.toString(), "nop");
}

TEST(BasicInstructionTest, EqualsOnOpcode)
{
    BasicInstruction a(NOP);
    BasicInstruction b(NOP);
    BasicInstruction c(ACONST_NULL);
    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
}

// ============== IntOperandInstruction ==============

TEST(IntOperandInstructionTest, Size1ByteOpcodes)
{
    // 1 字节操作数：BIPUSH, ILOAD-LSTORE, ALOAD-ASTORE, DLOAD-DSTORE, FLOAD-FSTORE, RET, NEWARRAY
    IntOperandInstruction bipush(BIPUSH, 0x42);
    EXPECT_EQ(bipush.computeSize(), 2);
    EXPECT_EQ(bipush.getOperand(), 0x42);

    IntOperandInstruction iload(ILOAD, 3);
    EXPECT_EQ(iload.computeSize(), 2);

    IntOperandInstruction ret(RET, 1);
    EXPECT_EQ(ret.computeSize(), 2);

    IntOperandInstruction newarray(NEWARRAY, 4);
    EXPECT_EQ(newarray.computeSize(), 2);
}

TEST(IntOperandInstructionTest, Size2ByteOpcodes)
{
    // 2 字节操作数：SIPUSH, IF*-IF*, GOTO, JSR, IFNULL, IFNONNULL
    IntOperandInstruction sipush(SIPUSH, 0x1234);
    EXPECT_EQ(sipush.computeSize(), 3);

    IntOperandInstruction ifeq(IFEQ, 100);
    EXPECT_EQ(ifeq.computeSize(), 3);

    IntOperandInstruction goto_(GOTO, -1);
    EXPECT_EQ(goto_.computeSize(), 3);

    IntOperandInstruction ifnull(IFNULL, 50);
    EXPECT_EQ(ifnull.computeSize(), 3);
}

TEST(IntOperandInstructionTest, Size4ByteOpcodes)
{
    IntOperandInstruction goto_w(GOTO_W, 0x12345678);
    EXPECT_EQ(goto_w.computeSize(), 5);

    IntOperandInstruction jsr_w(JSR_W, -1);
    EXPECT_EQ(jsr_w.computeSize(), 5);
}

TEST(IntOperandInstructionTest, Equals)
{
    IntOperandInstruction a(SIPUSH, 0x1234);
    IntOperandInstruction b(SIPUSH, 0x1234);
    IntOperandInstruction c(SIPUSH, 0x1235);
    IntOperandInstruction d(BIPUSH, 0x1234);   // 不同 opcode
    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
    EXPECT_FALSE(a.equals(d));
}

// ============== IincInstruction ==============

TEST(IincInstructionTest, Basic)
{
    IincInstruction i(5, -3);
    EXPECT_EQ(i.kind(), InsnKind::Iinc);
    EXPECT_EQ(i.getOpcode(), IINC);
    EXPECT_EQ(i.getVar(), 5);
    EXPECT_EQ(i.getIncrement(), -3);
    EXPECT_EQ(i.computeSize(), 3);
    EXPECT_NE(i.toString().find("var=5"), std::string::npos);
    EXPECT_NE(i.toString().find("incr=-3"), std::string::npos);
}

TEST(IincInstructionTest, Equals)
{
    IincInstruction a(5, -3);
    IincInstruction b(5, -3);
    IincInstruction c(6, -3);
    IincInstruction d(5, 2);
    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
    EXPECT_FALSE(a.equals(d));
}

// ============== CpRefInstruction ==============

TEST(CpRefInstructionTest, KindAndEmptyCpAccesses)
{
    CpUtf8           utf8("Foo");
    CpRefInstruction ins(LDC, &utf8);
    EXPECT_EQ(ins.kind(), InsnKind::CpRef);
    EXPECT_EQ(ins.getOpcode(), LDC);
    EXPECT_EQ(ins.computeSize(), 2);
    EXPECT_EQ(ins.getEntry(), &utf8);
    auto accesses = ins.cpAccesses();
    ASSERT_EQ(accesses.size(), 1u);
    EXPECT_EQ(accesses[0], &utf8);
}

TEST(CpRefInstructionTest, Size2Opcodes)
{
    CpUtf8           utf8("X");
    CpRefInstruction ldc(LDC, &utf8);
    EXPECT_EQ(ldc.computeSize(), 2);
}

TEST(CpRefInstructionTest, Size5OpcodesForInvokeWithPadding)
{
    CpUtf8           utf8("X");
    CpRefInstruction idy(INVOKEDYNAMIC, &utf8);
    EXPECT_EQ(idy.computeSize(), 5);
    CpRefInstruction ii(INVOKEINTERFACE, &utf8);
    EXPECT_EQ(ii.computeSize(), 5);
}

TEST(CpRefInstructionTest, Size3OpcodesForOtherCpRefs)
{
    CpUtf8           utf8("X");
    CpRefInstruction ldcw(LDC_W, &utf8);
    EXPECT_EQ(ldcw.computeSize(), 3);
    CpRefInstruction ldc2w(LDC2_W, &utf8);
    EXPECT_EQ(ldc2w.computeSize(), 3);
    CpRefInstruction getstatic(GETSTATIC, &utf8);
    EXPECT_EQ(getstatic.computeSize(), 3);
    CpRefInstruction invokespecial(INVOKESPECIAL, &utf8);
    EXPECT_EQ(invokespecial.computeSize(), 3);
    CpRefInstruction anewarray(ANEWARRAY, &utf8);
    EXPECT_EQ(anewarray.computeSize(), 3);
}

TEST(CpRefInstructionTest, Equals)
{
    CpUtf8           utf8a("A");
    CpUtf8           utf8b("B");
    CpRefInstruction a(LDC, &utf8a);
    CpRefInstruction b(LDC, &utf8a);
    CpRefInstruction c(LDC, &utf8b);
    CpRefInstruction d(LDC_W, &utf8a);   // 不同 opcode
    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
    EXPECT_FALSE(a.equals(d));
}

// ============== TableSwitchInstruction ==============

TEST(TableSwitchInstructionTest, NotifyStartPositionPadding)
{
    // padding = 3 - (position & 3)
    TableSwitchInstruction ts(0, 0, 2, {10, 20, 30});
    EXPECT_EQ(ts.getPadding(), -1);    // 初始未设置
    EXPECT_EQ(ts.computeSize(), -1);   // 抛错前的 sentinel

    ts.notifyStartPosition(0);
    EXPECT_EQ(ts.getPadding(), 3);
    EXPECT_EQ(ts.computeSize(), 1 + 3 + 4 + 4 + 4 + 4 * 3);   // 28

    ts.notifyStartPosition(1);
    EXPECT_EQ(ts.getPadding(), 2);
    EXPECT_EQ(ts.computeSize(), 1 + 2 + 4 + 4 + 4 + 4 * 3);   // 27

    ts.notifyStartPosition(3);
    EXPECT_EQ(ts.getPadding(), 0);
    EXPECT_EQ(ts.computeSize(), 1 + 0 + 4 + 4 + 4 + 4 * 3);   // 25
}

// ============== LookupSwitchInstruction ==============

TEST(LookupSwitchInstructionTest, NotifyStartPositionPadding)
{
    LookupSwitchInstruction ls(0, {1, 2, 3}, {10, 20, 30});
    EXPECT_EQ(ls.getPadding(), -1);

    ls.notifyStartPosition(0);
    EXPECT_EQ(ls.getPadding(), 3);
    // u1 opcode + 3 pad + u4 dflt + u4 npairs + 3 * (u4 key + u4 offset)
    EXPECT_EQ(ls.computeSize(), 1 + 3 + 4 + 4 + 3 * 8);   // 36

    ls.notifyStartPosition(3);
    EXPECT_EQ(ls.getPadding(), 0);
    EXPECT_EQ(ls.computeSize(), 1 + 0 + 4 + 4 + 3 * 8);   // 33
}

// ============== MultiANewArrayInstruction ==============

TEST(MultiANewArrayInstructionTest, Basic)
{
    CpUtf8                    name("I");
    CpClass                   clazz(&name);
    MultiANewArrayInstruction m(&clazz, 3);
    EXPECT_EQ(m.kind(), InsnKind::MultiANewArray);
    EXPECT_EQ(m.getOpcode(), MULTIANEWARRAY);
    EXPECT_EQ(m.getDimensions(), 3);
    EXPECT_EQ(m.getDescriptor(), &clazz);
    EXPECT_EQ(m.computeSize(), 4);

    auto accesses = m.cpAccesses();
    ASSERT_EQ(accesses.size(), 1u);
    EXPECT_EQ(accesses[0], &clazz);
}

// ============== WideInstruction ==============

TEST(WideInstructionTest, WrapsLoad)
{
    auto            backing = std::make_unique<IntOperandInstruction>(ILOAD, 5);
    WideInstruction wide(std::move(backing));
    EXPECT_EQ(wide.kind(), InsnKind::Wide);
    EXPECT_EQ(wide.getOpcode(), WIDE);
    EXPECT_EQ(wide.computeSize(), 4);   // wide + backing 4 字节
    EXPECT_NE(wide.getBacking(), nullptr);
}

TEST(WideInstructionTest, WrapsIinc6Bytes)
{
    auto            backing = std::make_unique<IincInstruction>(200, 1000);
    WideInstruction wide(std::move(backing));
    EXPECT_EQ(wide.computeSize(), 6);   // wide + iinc 6 字节特殊
}

TEST(WideInstructionTest, CpAccessesDelegated)
{
    CpUtf8          utf8("X");
    auto            backing = std::make_unique<CpRefInstruction>(LDC, &utf8);
    WideInstruction wide(std::move(backing));
    auto            accesses = wide.cpAccesses();
    ASSERT_EQ(accesses.size(), 1u);
    EXPECT_EQ(accesses[0], &utf8);
}

TEST(WideInstructionTest, Equals)
{
    auto            a_backing = std::make_unique<IntOperandInstruction>(ILOAD, 5);
    auto            b_backing = std::make_unique<IntOperandInstruction>(ILOAD, 5);
    WideInstruction a(std::move(a_backing));
    WideInstruction b(std::move(b_backing));
    EXPECT_TRUE(a.equals(b));
}

// ============== Kind 分派（替代 Java instanceof） ==============

TEST(InstructionKindTest, KindDistinguishes)
{
    BasicInstruction          basic(NOP);
    IntOperandInstruction     ioi(SIPUSH, 1);
    IincInstruction           iinc(0, 1);
    CpUtf8                    utf8("X");
    CpRefInstruction          cri(LDC, &utf8);
    TableSwitchInstruction    tsi(0, 0, 0, {});
    LookupSwitchInstruction   lsi(0, {}, {});
    CpClass                   clazz(&utf8);
    MultiANewArrayInstruction mnai(&clazz, 1);
    WideInstruction           wide(std::make_unique<BasicInstruction>(NOP));

    EXPECT_EQ(basic.kind(), InsnKind::Basic);
    EXPECT_EQ(ioi.kind(), InsnKind::IntOperand);
    EXPECT_EQ(iinc.kind(), InsnKind::Iinc);
    EXPECT_EQ(cri.kind(), InsnKind::CpRef);
    EXPECT_EQ(tsi.kind(), InsnKind::TableSwitch);
    EXPECT_EQ(lsi.kind(), InsnKind::LookupSwitch);
    EXPECT_EQ(mnai.kind(), InsnKind::MultiANewArray);
    EXPECT_EQ(wide.kind(), InsnKind::Wide);
}
