// 安全审计修复的边界测试：tableswitch/lookupswitch 的数量字段（high-low+1、
// npairs）由敌意 class 控制，旧实现直接 reserve(count) —— 负数经 (size_t)
// 提升会触发天文预分配。修复后：负数显式拒绝（对应 Java
// NegativeArraySizeException）；reserve 按底层缓冲剩余字节钳制；数量超过
// 缓冲供给时由 require() 的 out_of_range 自然终止（保持上游 i32 回绕与
// 溢读语义，crasher 语料兼容）。

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "jclassfile/io/indexable_byte_stream.hpp"
#include "jclassfile/io/instruction_reader.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"
#include "libca/core/datatype.hpp"

using namespace jclassfile;
using namespace jclassfile::io;
using namespace jclassfile::model;
using namespace jclassfile::model::Opcodes;

namespace {

std::unique_ptr<ConstPool> makeMinimalPool()
{
    auto pool = std::make_unique<ConstPool>();
    pool->add(std::make_unique<CpUtf8>("Foo"));
    return pool;
}

void putU4(std::vector<ca::u8>& out, ca::u32 value)
{
    out.push_back(static_cast<ca::u8>((value >> 24) & 0xFF));
    out.push_back(static_cast<ca::u8>((value >> 16) & 0xFF));
    out.push_back(static_cast<ca::u8>((value >> 8) & 0xFF));
    out.push_back(static_cast<ca::u8>(value & 0xFF));
}

}   // namespace

// low=0, high=0x7FFFFFF0：i64 化之前的 high - low + 1 接近 2^31，声明 case 数
// 与 code 剩余 16 字节完全不成比例，必须快速抛出而不是 reserve 爆内存。
TEST(InstructionReaderBounds, TableSwitchCountBeyondCodeBytesThrows)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {TABLESWITCH, 0x00, 0x00, 0x00};   // opcode + 3 padding
    putU4(code, 0);             // dflt
    putU4(code, 0);             // low
    putU4(code, 0x7FFFFFF0u);   // high
    IndexableByteStream is(code);

    InstructionReader reader;
    EXPECT_THROW((void)reader.read(is, *pool, static_cast<int>(code.size())), std::out_of_range);
}

// low > high：high - low + 1 为负，直接拒绝。
TEST(InstructionReaderBounds, TableSwitchNegativeCountThrows)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {TABLESWITCH, 0x00, 0x00, 0x00};
    putU4(code, 0);   // dflt
    putU4(code, 5);   // low
    putU4(code, 1);   // high < low
    IndexableByteStream is(code);

    InstructionReader reader;
    EXPECT_THROW((void)reader.read(is, *pool, static_cast<int>(code.size())), std::runtime_error);
}

// npairs = 0x7FFFFFFF：旧实现 keys/offsets 各 reserve 一次天文数字。
TEST(InstructionReaderBounds, LookupSwitchKeyCountBeyondCodeBytesThrows)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {LOOKUPSWITCH, 0x00, 0x00, 0x00};   // opcode + 3 padding
    putU4(code, 0);              // dflt
    putU4(code, 0x7FFFFFFFu);    // npairs
    IndexableByteStream is(code);

    InstructionReader reader;
    EXPECT_THROW((void)reader.read(is, *pool, static_cast<int>(code.size())), std::out_of_range);
}

// npairs 为负：直接拒绝。
TEST(InstructionReaderBounds, LookupSwitchNegativeKeyCountThrows)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {LOOKUPSWITCH, 0x00, 0x00, 0x00};
    putU4(code, 0);              // dflt
    putU4(code, 0xFFFFFFFEu);    // npairs = -2
    IndexableByteStream is(code);

    InstructionReader reader;
    EXPECT_THROW((void)reader.read(is, *pool, static_cast<int>(code.size())), std::runtime_error);
}

// 边界内的合法 switch 不受封顶影响：3 项 tableswitch 正常读出。
TEST(InstructionReaderBounds, TableSwitchWithinBoundsStillReads)
{
    auto                pool = makeMinimalPool();
    std::vector<ca::u8> code = {TABLESWITCH, 0x00, 0x00, 0x00};
    putU4(code, 20);   // dflt
    putU4(code, 1);    // low
    putU4(code, 3);    // high
    putU4(code, 5);    // offset case 1
    putU4(code, 10);   // offset case 2
    putU4(code, 15);   // offset case 3
    IndexableByteStream is(code);

    InstructionReader reader;
    auto              ins = reader.read(is, *pool, static_cast<int>(code.size()));
    ASSERT_EQ(ins.size(), 1u);
    EXPECT_EQ(ins[0]->getOpcode(), TABLESWITCH);
    EXPECT_EQ(ins[0]->computeSize(), static_cast<int>(code.size()));
}
