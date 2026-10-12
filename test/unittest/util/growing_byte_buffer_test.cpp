// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/util/GrowingByteBuffer.java
// CAFED00D 原版未直接为 GrowingByteBuffer 建单测（无 GrowingByteBufferTest.java）。
// 本测试覆盖 CAFED00D 内部使用到的所有 put* API（AttributeWriter / InstructionWriter 用法）。

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include "libca/core/datatype.hpp"
#include "jclassfile/util/growing_byte_buffer.hpp"

using jclassfile::util::GrowingByteBuffer;
using ca::u8;

namespace {

std::vector<u8> bytes(const GrowingByteBuffer& buf)
{
    auto& b = buf.unwrap();
    return std::vector<u8>(b.begin(), b.begin() + buf.position());
}

}   // namespace

// 构造：初始 position == 0，capacity >= 0，underflow 数据全为 0。
TEST(GrowingByteBufferTest, ConstructStartsAtZero)
{
    GrowingByteBuffer buf;
    EXPECT_EQ(buf.position(), 0);
    EXPECT_EQ(buf.unwrap().size(), 0u);
}

// put(value)：写入 1 字节（低 8 位），position += 1。
TEST(GrowingByteBufferTest, PutWritesOneByte)
{
    GrowingByteBuffer buf;
    buf.put(0xAB);
    EXPECT_EQ(buf.position(), 1);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], 0xABu);
}

// put 高位截断：put(0x1FF) → 0xFF（与 Java (byte) 截断一致）。
TEST(GrowingByteBufferTest, PutTruncatesToByte)
{
    GrowingByteBuffer buf;
    buf.put(0x1FF);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], 0xFFu);
}

// putShort(value)：big-endian 2 字节。
TEST(GrowingByteBufferTest, PutShortBigEndian)
{
    GrowingByteBuffer buf;
    buf.putShort(0x1234);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 2u);
    EXPECT_EQ(data[0], 0x12u);
    EXPECT_EQ(data[1], 0x34u);
}

// putShort 高位截断：putShort(0x123456) → 0x3456。
TEST(GrowingByteBufferTest, PutShortTruncatesToShort)
{
    GrowingByteBuffer buf;
    buf.putShort(0x123456);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 2u);
    EXPECT_EQ(data[0], 0x34u);
    EXPECT_EQ(data[1], 0x56u);
}

// putInt(value)：big-endian 4 字节。
TEST(GrowingByteBufferTest, PutIntBigEndian)
{
    GrowingByteBuffer buf;
    buf.putInt(0x12345678);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 4u);
    EXPECT_EQ(data[0], 0x12u);
    EXPECT_EQ(data[1], 0x34u);
    EXPECT_EQ(data[2], 0x56u);
    EXPECT_EQ(data[3], 0x78u);
}

// putInt 负数：-1 → 0xFFFFFFFF（2's complement）。
TEST(GrowingByteBufferTest, PutIntNegative)
{
    GrowingByteBuffer buf;
    buf.putInt(-1);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 4u);
    EXPECT_EQ(data[0], 0xFFu);
    EXPECT_EQ(data[1], 0xFFu);
    EXPECT_EQ(data[2], 0xFFu);
    EXPECT_EQ(data[3], 0xFFu);
}

// putLong(value)：big-endian 8 字节。
TEST(GrowingByteBufferTest, PutLongBigEndian)
{
    GrowingByteBuffer buf;
    buf.putLong(0x0123456789ABCDEFLL);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 8u);
    EXPECT_EQ(data[0], 0x01u);
    EXPECT_EQ(data[1], 0x23u);
    EXPECT_EQ(data[2], 0x45u);
    EXPECT_EQ(data[3], 0x67u);
    EXPECT_EQ(data[4], 0x89u);
    EXPECT_EQ(data[5], 0xABu);
    EXPECT_EQ(data[6], 0xCDu);
    EXPECT_EQ(data[7], 0xEFu);
}

// putFloat：IEEE 754 4 字节。
TEST(GrowingByteBufferTest, PutFloatIeee754)
{
    GrowingByteBuffer buf;
    buf.putFloat(1.0f);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 4u);
    EXPECT_EQ(data[0], 0x3Fu);
    EXPECT_EQ(data[1], 0x80u);
    EXPECT_EQ(data[2], 0x00u);
    EXPECT_EQ(data[3], 0x00u);
}

// putDouble：IEEE 754 8 字节。
TEST(GrowingByteBufferTest, PutDoubleIeee754)
{
    GrowingByteBuffer buf;
    buf.putDouble(1.0);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 8u);
    EXPECT_EQ(data[0], 0x3Fu);
    EXPECT_EQ(data[1], 0xF0u);
    for (int i = 2; i < 8; ++i) {
        EXPECT_EQ(data[static_cast<size_t>(i)], 0x00u) << "byte " << i;
    }
}

// skip(N)：position += N，确保可写入。
TEST(GrowingByteBufferTest, SkipAdvancesPosition)
{
    GrowingByteBuffer buf;
    buf.skip(5);
    EXPECT_EQ(buf.position(), 5);
    // 后续 put 应在 offset 5 写
    buf.put(0xCC);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 6u);
    EXPECT_EQ(data[5], 0xCCu);
}

// ensureWriteable 触发扩容：超过初始 capacity 时 resize。
TEST(GrowingByteBufferTest, EnsureWriteableGrowsCapacity)
{
    GrowingByteBuffer buf;
    int               initial = static_cast<int>(buf.unwrap().size());
    buf.ensureWriteable(2048);
    EXPECT_GE(static_cast<int>(buf.unwrap().size()), initial + 2048);
    EXPECT_EQ(buf.position(), 0);
}

// 链式调用：put → putShort → putInt 返回 *this 引用。
TEST(GrowingByteBufferTest, ChainedCallsWork)
{
    GrowingByteBuffer buf;
    buf.put(0x01).putShort(0x0203).putInt(0x04050607);
    auto data = bytes(buf);
    ASSERT_EQ(data.size(), 7u);
    EXPECT_EQ(data[0], 0x01u);
    EXPECT_EQ(data[1], 0x02u);
    EXPECT_EQ(data[2], 0x03u);
    EXPECT_EQ(data[3], 0x04u);
    EXPECT_EQ(data[4], 0x05u);
    EXPECT_EQ(data[5], 0x06u);
    EXPECT_EQ(data[6], 0x07u);
}
