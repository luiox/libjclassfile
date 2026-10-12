// 阶段 5b：InstructionReader + InstructionWriter 端到端测试
// 覆盖：
//   - 单条 BasicInstruction round-trip（read + writeCode）
//   - IntOperandInstruction（BIPUSH / GOTO_W）
//   - IincInstruction
//   - CpRefInstruction（LDC / GETSTATIC / NEW / MULTIANEWARRAY）
//   - TABLESWITCH padding 计算
//   - LOOKUPSWITCH padding 计算
//   - WIDE 嵌套（包装 ILOAD 和 IINC）
//   - INVOKEINTERFACE 写 5 字节
//   - 多个指令序列 round-trip
//   - size 校验：reader 输出的 computeSize == writer 实际写入的字节数

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "jclassfile/io/indexable_byte_stream.hpp"
#include "jclassfile/io/instruction_reader.hpp"
#include "jclassfile/io/instruction_writer.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/attribute_constants.hpp"
#include "libca/core/datatype.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/const_ref.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_method_ref.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
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

using namespace jclassfile;
using namespace jclassfile::io;
using namespace jclassfile::model;
using namespace jclassfile::model::Opcodes;

namespace {

// 构造一个最小常量池：index 0 = ImplZero, index 1 = CpUtf8 "Foo"
std::unique_ptr<ConstPool> makeMinimalPool()
{
    auto pool = std::make_unique<ConstPool>();
    pool->add(std::make_unique<CpUtf8>("Foo"));
    return pool;
}

// 构造支持 CpRef / MultiANewArray / InvokeInterface 指令的常量池
// 索引分配：0=ImplZero, 1=Utf8 Object, 2=Class Object, 3=Utf8 equals,
//          4=Utf8 (Ljava/lang/Object;)Z, 5=NameType, 6=MethodRef
struct PoolWithRefs
{
    std::unique_ptr<ConstPool> pool;
    const CpClass*             classRef = nullptr;
};

PoolWithRefs makePoolWithRefs()
{
    PoolWithRefs out;
    out.pool = std::make_unique<ConstPool>();
    // 1: CpUtf8 "Ljava/lang/Object;"
    out.pool->add(std::make_unique<CpUtf8>("Ljava/lang/Object;"));
    // 2: CpClass -> #1
    out.classRef = static_cast<const CpClass*>(out.pool->get(2));
    out.pool->add(std::make_unique<CpClass>(static_cast<const CpUtf8*>(out.pool->get(1))));
    // 3: CpUtf8 "equals"
    out.pool->add(std::make_unique<CpUtf8>("equals"));
    // 4: CpUtf8 "(Ljava/lang/Object;)Z"
    out.pool->add(std::make_unique<CpUtf8>("(Ljava/lang/Object;)Z"));
    // 5: CpNameType -> #3, #4
    out.pool->add(std::make_unique<CpNameType>(static_cast<const CpUtf8*>(out.pool->get(3)),
                                               static_cast<const CpUtf8*>(out.pool->get(4))));
    // 6: CpMethodRef -> #2, #5
    out.pool->add(std::make_unique<CpMethodRef>(static_cast<const CpClass*>(out.pool->get(2)),
                                                static_cast<const CpNameType*>(out.pool->get(5))));
    return out;
}

}   // namespace

// ============== BasicInstruction round-trip ==============

TEST(InstructionRoundTrip, Nop)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {NOP};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, 1);
    ASSERT_EQ(ins.size(), 1u);
    EXPECT_EQ(ins[0]->getOpcode(), NOP);
    EXPECT_EQ(ins[0]->kind(), InsnKind::Basic);

    std::vector<const Instruction*> list = {ins[0].get()};
    InstructionWriter               writer;
    auto                            bytes = writer.writeCode(list);
    ASSERT_EQ(bytes.size(), 1u);
    EXPECT_EQ(bytes[0], NOP);
}

TEST(InstructionRoundTrip, Iconst5)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {ICONST_5};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, 1);
    EXPECT_EQ(ins[0]->getOpcode(), ICONST_5);

    InstructionWriter writer;
    auto              bytes = writer.writeCode({ins[0].get()});
    ASSERT_EQ(bytes.size(), 1u);
    EXPECT_EQ(bytes[0], ICONST_5);
}

// ============== IntOperandInstruction round-trip ==============

TEST(InstructionRoundTrip, Bipush)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {BIPUSH, 0x42};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, 2);
    EXPECT_EQ(ins[0]->getOpcode(), BIPUSH);
    auto* i = static_cast<IntOperandInstruction*>(ins[0].get());
    EXPECT_EQ(i->getOperand(), 0x42);
    EXPECT_EQ(ins[0]->computeSize(), 2);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

TEST(InstructionRoundTrip, Sipush)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {SIPUSH, 0x12, 0x34};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, 3);
    EXPECT_EQ(ins[0]->getOpcode(), SIPUSH);
    auto* i = static_cast<IntOperandInstruction*>(ins[0].get());
    EXPECT_EQ(i->getOperand(), 0x1234);
    EXPECT_EQ(ins[0]->computeSize(), 3);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

TEST(InstructionRoundTrip, GotoW)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {GOTO_W, 0x00, 0x00, 0x10, 0x00};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, 5);
    EXPECT_EQ(ins[0]->getOpcode(), GOTO_W);
    EXPECT_EQ(ins[0]->computeSize(), 5);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

TEST(InstructionRoundTrip, Iload)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {ILOAD, 0x03};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, 2);
    EXPECT_EQ(ins[0]->getOpcode(), ILOAD);
    EXPECT_EQ(ins[0]->computeSize(), 2);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

// ============== IincInstruction round-trip ==============

TEST(InstructionRoundTrip, Iinc)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {IINC, 0x05, 0xFD};   // var=5, incr=-3
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, 3);
    EXPECT_EQ(ins[0]->getOpcode(), IINC);
    auto* i = static_cast<IincInstruction*>(ins[0].get());
    EXPECT_EQ(i->getVar(), 5);
    EXPECT_EQ(i->getIncrement(), -3);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

// ============== CpRefInstruction round-trip ==============

TEST(InstructionRoundTrip, Ldc)
{
    auto refs = makePoolWithRefs();
    // LDC index=1 (CpUtf8 "Ljava/lang/Object;") — 任意 CpEntry 都可以做 loadable 的简单测试
    std::vector<ca::u8> code = {LDC, 0x01};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *refs.pool, 2);
    EXPECT_EQ(ins[0]->getOpcode(), LDC);
    EXPECT_EQ(ins[0]->computeSize(), 2);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

TEST(InstructionRoundTrip, Getstatic)
{
    auto refs = makePoolWithRefs();
    // GETSTATIC index=6 (CpMethodRef, but as 16-bit index = 0x0006)
    std::vector<ca::u8> code = {GETSTATIC, 0x00, 0x06};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *refs.pool, 3);
    EXPECT_EQ(ins[0]->getOpcode(), GETSTATIC);
    EXPECT_EQ(ins[0]->computeSize(), 3);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

TEST(InstructionRoundTrip, New)
{
    auto refs = makePoolWithRefs();
    // NEW index=2 (CpClass)
    std::vector<ca::u8> code = {NEW, 0x00, 0x02};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *refs.pool, 3);
    EXPECT_EQ(ins[0]->getOpcode(), NEW);
    EXPECT_EQ(ins[0]->computeSize(), 3);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

TEST(InstructionRoundTrip, Multianewarray)
{
    auto refs = makePoolWithRefs();
    // MULTIANEWARRAY index=2 dims=3
    std::vector<ca::u8> code = {MULTIANEWARRAY, 0x00, 0x02, 0x03};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *refs.pool, 4);
    EXPECT_EQ(ins[0]->getOpcode(), MULTIANEWARRAY);
    EXPECT_EQ(ins[0]->computeSize(), 4);
    auto* m = static_cast<MultiANewArrayInstruction*>(ins[0].get());
    EXPECT_EQ(m->getDimensions(), 3);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

// ============== INVOKEINTERFACE 5-byte round-trip ==============

TEST(InstructionRoundTrip, InvokeInterface)
{
    auto refs = makePoolWithRefs();
    // INVOKEINTERFACE index=6 (CpMethodRef) + nargs(0)+1=1, 0 padding
    // Parameter type of equals is (Ljava/lang/Object;)Z → 1 slot
    // nargs = 1 (slot) + 1 (this) = 2
    std::vector<ca::u8> code = {INVOKEINTERFACE, 0x00, 0x06, 0x02, 0x00};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *refs.pool, 5);
    EXPECT_EQ(ins[0]->getOpcode(), INVOKEINTERFACE);
    EXPECT_EQ(ins[0]->computeSize(), 5);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

// ============== TABLESWITCH padding 计算 + round-trip ==============

TEST(InstructionRoundTrip, TableSwitch)
{
    auto pool = makeMinimalPool();
    // TABLESWITCH: opcode=170 + 3 padding + dflt + low + high + 3 offsets
    //   = 1 + 3 + 4*5 = 24 字节
    std::vector<ca::u8> code = {TABLESWITCH, 0x00, 0x00, 0x00,   // 3 padding bytes
                                0x00,        0x00, 0x00, 0x14,   // dflt=20
                                0x00,        0x00, 0x00, 0x01,   // low=1
                                0x00,        0x00, 0x00, 0x03,   // high=3
                                0x00,        0x00, 0x00, 0x05, 0x00, 0x00,
                                0x00,        0x0A, 0x00, 0x00, 0x00, 0x0F};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, static_cast<int>(code.size()));
    EXPECT_EQ(ins[0]->getOpcode(), TABLESWITCH);
    EXPECT_EQ(ins[0]->computeSize(), static_cast<int>(code.size()));

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

// ============== LOOKUPSWITCH padding 计算 + round-trip ==============

TEST(InstructionRoundTrip, LookupSwitch)
{
    auto pool = makeMinimalPool();
    // LOOKUPSWITCH: opcode=171 + 3 padding + dflt + npairs + 3*(key+offset)
    //   = 1 + 3 + 4*1 + 4*1 + 3*8 = 40
    std::vector<ca::u8> code = {LOOKUPSWITCH, 0x00, 0x00, 0x00,   // 3 padding
                                0x00,         0x00, 0x00, 0x20,   // dflt=32
                                0x00,         0x00, 0x00, 0x03,   // npairs=3
                                0x00,         0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05,
                                0x00,         0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0A,
                                0x00,         0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0F};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, static_cast<int>(code.size()));
    EXPECT_EQ(ins[0]->getOpcode(), LOOKUPSWITCH);
    EXPECT_EQ(ins[0]->computeSize(), static_cast<int>(code.size()));

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

// ============== WIDE 嵌套 round-trip ==============

TEST(InstructionRoundTrip, WideIload)
{
    auto pool = makeMinimalPool();
    // WIDE ILOAD index=200 → 1 (wide) + 1 (iload) + 2 (index) = 4
    std::vector<ca::u8> code = {WIDE, ILOAD, 0x00, 0xC8};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, 4);
    EXPECT_EQ(ins[0]->getOpcode(), WIDE);
    EXPECT_EQ(ins[0]->computeSize(), 4);
    auto* w = static_cast<WideInstruction*>(ins[0].get());
    ASSERT_NE(w->getBacking(), nullptr);
    EXPECT_EQ(w->getBacking()->getOpcode(), ILOAD);
    auto* b = static_cast<IntOperandInstruction*>(w->getBacking());
    EXPECT_EQ(b->getOperand(), 0x00C8);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

TEST(InstructionRoundTrip, WideIinc)
{
    auto pool = makeMinimalPool();
    // WIDE IINC var=200 incr=1000 → 1+1+2+2 = 6
    std::vector<ca::u8> code = {WIDE, IINC, 0x00, 0xC8, 0x03, 0xE8};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, 6);
    EXPECT_EQ(ins[0]->getOpcode(), WIDE);
    EXPECT_EQ(ins[0]->computeSize(), 6);

    auto bytes = InstructionWriter().writeCode({ins[0].get()});
    EXPECT_EQ(bytes, code);
}

// ============== 多指令序列 round-trip ==============

TEST(InstructionRoundTrip, MixedSequence)
{
    auto refs = makePoolWithRefs();
    // NOP, ICONST_0, ICONST_1, IADD, RETURN
    std::vector<ca::u8> code = {NOP, ICONST_0, ICONST_1, IADD, RETURN};
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *refs.pool, 5);
    ASSERT_EQ(ins.size(), 5u);
    EXPECT_EQ(ins[0]->getOpcode(), NOP);
    EXPECT_EQ(ins[1]->getOpcode(), ICONST_0);
    EXPECT_EQ(ins[2]->getOpcode(), ICONST_1);
    EXPECT_EQ(ins[3]->getOpcode(), IADD);
    EXPECT_EQ(ins[4]->getOpcode(), RETURN);

    std::vector<const Instruction*> list;
    for (auto& p : ins) list.push_back(p.get());
    auto bytes = InstructionWriter().writeCode(list);
    EXPECT_EQ(bytes, code);
}

// ============== size 校验 ==============

TEST(InstructionWriterTest, SizeMatchesCompute)
{
    auto                refs = makePoolWithRefs();
    std::vector<ca::u8> code = {NOP, ICONST_0, IADD, ICONST_5, RETURN};
    IndexableByteStream is(code);

    InstructionReader               reader;
    auto                            ins = reader.read(is, *refs.pool, 5);
    std::vector<const Instruction*> list;
    for (auto& p : ins) list.push_back(p.get());
    auto bytes = InstructionWriter().writeCode(list);
    EXPECT_EQ(bytes.size(), code.size());
}

namespace {

class MarkerFallbackWriter final : public FallbackInstructionWriter
{
public:
    void write(const Instruction& instruction, jclassfile::util::GrowingByteBuffer& buffer) override
    {
        sawCall = true;
        (void)instruction;
        buffer.put(0x7F);
    }

    bool sawCall = false;
};

class MarkerClassWriter final : public ClassFileWriter
{
public:
    MarkerFallbackWriter* marker = nullptr;

protected:
    std::unique_ptr<FallbackInstructionWriter> newFallbackInstructionWriter(
        ClassFile& /*clazz*/) override
    {
        auto ptr = std::make_unique<MarkerFallbackWriter>();
        marker   = ptr.get();
        return ptr;
    }
};

class UnsupportedInstruction final : public Instruction
{
public:
    explicit UnsupportedInstruction(int opcode)
        : Instruction(opcode)
        , opcode_(opcode)
    {}

    [[nodiscard]] int      getOpcode() const noexcept { return opcode_; }
    [[nodiscard]] int      computeSize() const noexcept override { return 2; }
    [[nodiscard]] InsnKind kind() const noexcept override { return InsnKind::Basic; }

private:
    int opcode_;
};

}   // namespace

TEST(AttributeWriterTest, PropagatesFallbackWriterIntoNestedInstructionWriter)
{
    auto refs = makePoolWithRefs();
    refs.pool->add(std::make_unique<CpUtf8>(std::string(AttributeConstants::CODE)));
    auto* attrName = static_cast<const CpUtf8*>(refs.pool->get(7));

    std::vector<const Instruction*> insns;
    UnsupportedInstruction          badInsn(0xCA);
    insns.push_back(&badInsn);
    CodeAttribute code(attrName, 1, 1, insns, {}, {});

    auto            fallback = std::make_unique<MarkerFallbackWriter>();
    auto*           marker   = fallback.get();
    AttributeWriter writer(std::move(fallback));
    auto            bytes = writer.writeAttribute(code);
    EXPECT_FALSE(bytes.empty());
    EXPECT_TRUE(marker->sawCall);
}

TEST(InstructionWriterTest, NullFallbackUsesFailFastWriter)
{
    UnsupportedInstruction badInsn(0xCA);
    InstructionWriter      writer(static_cast<FallbackInstructionWriter*>(nullptr));

    EXPECT_THROW((void)writer.writeCode({&badInsn}), std::logic_error);
}
