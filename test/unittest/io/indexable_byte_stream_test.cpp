// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/IndexableByteStream.java
// CAFED00D 原版未直接为 IndexableByteStream 建单测（无 IndexableByteStreamTest.java）。
// 本测试覆盖 CAFED00D 内部使用到的所有 API（AttributeReader.java:120-124
// 等位置），逐方法标注原版调用方。

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>
#include <vector>

#include "jclassfile/io/indexable_byte_stream.hpp"

using jclassfile::io::IndexableByteStream;
using ca::u8;

namespace {

std::vector<u8> bytes(std::initializer_list<u8> init)
{
    return std::vector<u8>(init);
}

}   // namespace

// 构造 (byte[])：初始 absolute index == 0，getBuffer() == 整个数组。
TEST(IndexableByteStreamTest, ConstructFromBufferStartsAtZero)
{
    auto                data = bytes({0x01, 0x02, 0x03, 0x04});
    IndexableByteStream is(data);

    EXPECT_EQ(is.getAbsoluteIndex(), 0);
    EXPECT_EQ(is.getIndex(), 0);
    EXPECT_EQ(is.getBuffer(), data);
}

// moveToAbsolute：把 position_ 设为绝对位置。
// 对应原版 AttributeReader.java:123 is.moveToAbsolute(index + expectedContentLength)
TEST(IndexableByteStreamTest, MoveToAbsoluteAdvancesAbsoluteIndex)
{
    auto                data = bytes({0x00, 0x00, 0x00, 0x00, 0xAA});
    IndexableByteStream is(data);
    is.moveToAbsolute(4);
    EXPECT_EQ(is.getAbsoluteIndex(), 4);
    EXPECT_EQ(is.readUnsignedByte(), 0xAA);
}

// moveTo：把 position_ 设为相对 slice start 的位置。
TEST(IndexableByteStreamTest, MoveToRelativeToSliceStart)
{
    auto                data = bytes({0x11, 0x22, 0x33, 0x44, 0x55});
    IndexableByteStream is(data);
    is.moveTo(2);
    EXPECT_EQ(is.getAbsoluteIndex(), 2);
    EXPECT_EQ(is.readUnsignedByte(), 0x33);
}

// moveBack：把 position_ 减去 distance。
// 对应原版 ClassFileReader.java:94 is.moveBack(diff)
TEST(IndexableByteStreamTest, MoveBackReturnsBehindAbsolute)
{
    auto                data = bytes({0x00, 0x00, 0x00, 0x00, 0x00, 0xCA, 0xFE, 0xBA, 0xBE});
    IndexableByteStream is(data);
    is.moveToAbsolute(9);
    is.moveBack(4);
    EXPECT_EQ(is.getAbsoluteIndex(), 5);
    EXPECT_EQ(is.readUnsignedByte(), 0xCA);
}

// readUnsignedByte / readUnsignedShort / readInt：big-endian 读，position 推进。
// 对应原版 ClassFileReader.java:72 is.readInt() 读 magic
TEST(IndexableByteStreamTest, ReadUnsignedByteAdvancesPosition)
{
    auto                data = bytes({0x7F, 0xFF, 0x42});
    IndexableByteStream is(data);
    EXPECT_EQ(is.readUnsignedByte(), 0x7F);
    EXPECT_EQ(is.getAbsoluteIndex(), 1);
    EXPECT_EQ(is.readUnsignedByte(), 0xFF);
    EXPECT_EQ(is.readUnsignedByte(), 0x42);
    EXPECT_EQ(is.getAbsoluteIndex(), 3);
}

TEST(IndexableByteStreamTest, ReadUnsignedShortIsBigEndian)
{
    auto                data = bytes({0xCA, 0xFE});
    IndexableByteStream is(data);
    EXPECT_EQ(is.readUnsignedShort(), 0xCAFE);
    EXPECT_EQ(is.getAbsoluteIndex(), 2);
}

TEST(IndexableByteStreamTest, ReadIntIsBigEndianMatchesCafebabe)
{
    auto                data = bytes({0xCA, 0xFE, 0xBA, 0xBE});
    IndexableByteStream is(data);
    EXPECT_EQ(static_cast<unsigned>(is.readInt()), 0xCAFEBABEu);
}

// readFloat / readDouble：IEEE 754 big-endian 字节序。
TEST(IndexableByteStreamTest, ReadFloatIsBigEndianIeee754)
{
    auto                data = bytes({0x42, 0xF6, 0xE6, 0x66});
    IndexableByteStream is(data);
    EXPECT_FLOAT_EQ(is.readFloat(), 123.45f);
}

TEST(IndexableByteStreamTest, ReadDoubleIsBigEndianIeee754)
{
    // 1.0 = 0x3FF0 0000 0000 0000 (big-endian)
    auto                data = bytes({0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    IndexableByteStream is(data);
    EXPECT_DOUBLE_EQ(is.readDouble(), 1.0);
}

// readLong：8 字节 big-endian。
TEST(IndexableByteStreamTest, ReadLongIsBigEndian)
{
    auto                data = bytes({0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02});
    IndexableByteStream is(data);
    EXPECT_EQ(static_cast<unsigned long long>(is.readLong()), 0x100000002ULL);
}

// readUTF：u16 length + length bytes modified UTF-8。
// 对应原版 ClassFileReader.java:161 new CpUtf8(is.readUTF())
TEST(IndexableByteStreamTest, ReadUtfReadsLengthPrefixedBytes)
{
    std::string     text = "java/lang/Object";
    std::vector<u8> data;
    auto            len = static_cast<ca::u16>(text.size());
    data.push_back(static_cast<u8>((len >> 8) & 0xFF));
    data.push_back(static_cast<u8>(len & 0xFF));
    for (char c : text) data.push_back(static_cast<u8>(c));
    IndexableByteStream is(data);
    EXPECT_EQ(is.readUTF(), text);
}

TEST(IndexableByteStreamTest, ReadUtfDecodesModifiedUtf8NullAndSupplementaryCodePoint)
{
    std::vector<u8>     data = {0x00, 0x09, 0x41, 0xC0, 0x80, 0xED, 0xA0, 0xBD, 0xED, 0xB8, 0x80};
    IndexableByteStream is(data);
    EXPECT_EQ(is.readUTF(), std::string("A\0\xF0\x9F\x98\x80", 6));
}

TEST(IndexableByteStreamTest, ReadUtfRejectsMalformedFourByteUtf8Sequence)
{
    std::vector<u8>     data = {0x00, 0x04, 0xF0, 0x9F, 0x98, 0x80};
    IndexableByteStream is(data);
    EXPECT_THROW(is.readUTF(), std::runtime_error);
}

TEST(IndexableByteStreamTest, ReadMutf8RefReturnsUnderlyingSliceWithoutCopy)
{
    std::string     text = "abc";
    std::vector<u8> data;
    auto            len = static_cast<ca::u16>(text.size());
    data.push_back(static_cast<u8>((len >> 8) & 0xFF));
    data.push_back(static_cast<u8>(len & 0xFF));
    for (char c : text) data.push_back(static_cast<u8>(c));
    IndexableByteStream is(data);
    auto                view = is.readMutf8Ref();
    ASSERT_EQ(view.size(), text.size());
    EXPECT_EQ(std::string(view.data(), view.size()), text);
}

// slice：从父流创建子流，父流 absolute position 不变（slice 构造只读不消耗父流）。
// 对应原版 AttributeReader.java:122 this.is = new IndexableByteStream(is, expectedContentLength)
TEST(IndexableByteStreamTest, SliceCreatesSubStreamWithSameAbsoluteStart)
{
    auto                data = bytes({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
    IndexableByteStream parent(data);
    parent.moveToAbsolute(2);
    IndexableByteStream slice(parent, 3);

    EXPECT_EQ(slice.getAbsoluteIndex(), 2);
    EXPECT_EQ(slice.getIndex(), 0);
    EXPECT_EQ(slice.readUnsignedByte(), 0x03);
    EXPECT_EQ(slice.readUnsignedByte(), 0x04);
    EXPECT_EQ(slice.getIndex(), 2);
}

// slice 后父流 position 不受影响（slice 是观察，不是消耗）。
TEST(IndexableByteStreamTest, SliceDoesNotConsumeParentPosition)
{
    auto                data = bytes({0x01, 0x02, 0x03, 0x04, 0x05});
    IndexableByteStream parent(data);
    parent.moveToAbsolute(2);
    IndexableByteStream slice(parent, 2);
    (void)slice.readUnsignedByte();
    (void)slice.readUnsignedByte();
    EXPECT_EQ(parent.getAbsoluteIndex(), 2);
}

// slice 允许越过 declared 长度读到父流缓冲内的字节（-noverify 宽容语义），
// 越过整个缓冲仍由 require() 抛出。宽容是刻意契约：混淆语料中存在内容长度
// 与实际读取量不符的属性，严格拒绝会让异常在属性解析中途抛出、流停在与
// 内容长度不符的位置，后续属性从错误偏移级联误解析（crasher 语料依赖此语义）。
TEST(IndexableByteStreamTest, SliceReadPastSliceEndReadsParentBufferThenThrows)
{
    auto                data = bytes({0x01, 0x02, 0x03, 0x04});
    IndexableByteStream parent(data);
    parent.moveToAbsolute(1);
    IndexableByteStream slice(parent, 2);
    EXPECT_EQ(slice.readUnsignedByte(), 0x02);
    EXPECT_EQ(slice.readUnsignedByte(), 0x03);
    EXPECT_EQ(slice.readUnsignedByte(), 0x04);
    EXPECT_THROW((void)slice.readUnsignedByte(), std::out_of_range);
}

// 顶层流的 slice_end == 缓冲长度：恰好读到末尾合法，越过末尾抛出；
// readableBytesLeft 随读取推进递减，到末尾为 0。
TEST(IndexableByteStreamTest, TopLevelStreamBoundsAndReadableBytesLeft)
{
    auto                data = bytes({0x01, 0x02, 0x03});
    IndexableByteStream is(data);
    EXPECT_EQ(is.readableBytesLeft(), 3);
    EXPECT_EQ(is.readUnsignedByte(), 0x01);
    EXPECT_EQ(is.readableBytesLeft(), 2);
    is.skipBytes(2);
    EXPECT_EQ(is.readableBytesLeft(), 0);
    EXPECT_THROW((void)is.readUnsignedByte(), std::out_of_range);
}

// 子流的 readableBytesLeft 以 slice_end 为上界，而非整个父缓冲。
TEST(IndexableByteStreamTest, SliceReadableBytesLeftBoundedBySliceEnd)
{
    auto                data = bytes({0x01, 0x02, 0x03, 0x04});
    IndexableByteStream parent(data);
    parent.moveToAbsolute(1);
    IndexableByteStream slice(parent, 2);
    EXPECT_EQ(slice.readableBytesLeft(), 2);
    (void)slice.readUnsignedByte();
    EXPECT_EQ(slice.readableBytesLeft(), 1);
}

// moveToAbsolute 拒绝负绝对位置：敌意 attribute content length 会产生负位置，
// 负位置通过旧版 require() 上界检查后即负下标读（UB）。
TEST(IndexableByteStreamTest, MoveToAbsoluteNegativeThrows)
{
    auto                data = bytes({0x01, 0x02, 0x03});
    IndexableByteStream is(data);
    EXPECT_THROW(is.moveToAbsolute(-1), std::out_of_range);
    EXPECT_THROW(is.moveToAbsolute(std::numeric_limits<int>::min()), std::out_of_range);
}

// skipBytes 负长度：require() 拒绝。
TEST(IndexableByteStreamTest, SkipBytesNegativeThrows)
{
    auto                data = bytes({0x01, 0x02, 0x03});
    IndexableByteStream is(data);
    EXPECT_THROW(is.skipBytes(-1), std::out_of_range);
}

// getBuffer：未 slice 返回整个 buffer，slice 后返回 [slice_start, slice_end) 范围。
TEST(IndexableByteStreamTest, GetBufferReturnsSliceRange)
{
    auto                data = bytes({0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
    IndexableByteStream parent(data);
    parent.moveToAbsolute(2);
    IndexableByteStream slice(parent, 3);
    auto                buf      = slice.getBuffer();
    std::vector<u8>     expected = {0x03, 0x04, 0x05};
    EXPECT_EQ(buf, expected);
}

TEST(IndexableByteStreamTest, GetBufferReturnsFullForNonSliced)
{
    auto                data = bytes({0x01, 0x02, 0x03});
    IndexableByteStream is(data);
    EXPECT_EQ(is.getBuffer(), data);
}

// readFully：追加 n 字节到 dest，position 推进 n。
TEST(IndexableByteStreamTest, ReadFullyAppendsBytes)
{
    auto                data = bytes({0x01, 0x02, 0x03, 0x04});
    IndexableByteStream is(data);
    std::vector<u8>     dest;
    is.readFully(dest, 3);
    EXPECT_EQ(dest, (std::vector<u8>{0x01, 0x02, 0x03}));
    EXPECT_EQ(is.getAbsoluteIndex(), 3);
}

// 越界读抛 out_of_range。
TEST(IndexableByteStreamTest, ReadPastEndThrows)
{
    auto                data = bytes({0x01, 0x02});
    IndexableByteStream is(data);
    EXPECT_THROW(is.readInt(), std::out_of_range);
}
