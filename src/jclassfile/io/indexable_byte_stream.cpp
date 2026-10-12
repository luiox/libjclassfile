// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/IndexableByteStream.java

#include "indexable_byte_stream.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

#include "jclassfile/io/mutf8_codec.hpp"
#include "jstr/mutf8_string.hpp"

namespace jclassfile::io {

IndexableByteStream::IndexableByteStream(const std::vector<ca::u8>& data)
    : buffer_(&data)
    , position_(0)
    , slice_start_(0)
    // 顶层流覆盖整个缓冲区：slice_end_ 与 size_ 一致，require() 的子流上界
    // 检查在顶层流上等价于原有的缓冲区上界检查。
    , slice_end_(static_cast<int>(data.size()))
    , size_(static_cast<int>(data.size()))
{}

IndexableByteStream::IndexableByteStream(const IndexableByteStream& parent, int length)
    : buffer_(&parent.getUnderlyingBuffer())
    , position_(parent.getAbsoluteIndex())
    , slice_start_(parent.getAbsoluteIndex())
    // 敌意 attribute length 可达 INT_MAX：加法升到 i64 再钳制，避免 int 溢出回绕
    // 出负的 slice_end_（getBuffer 以倒置迭代器范围构造 vector 属 UB）。
    , slice_end_(static_cast<int>(std::min(static_cast<ca::i64>(parent.getAbsoluteIndex()) + length,
                                           static_cast<ca::i64>(parent.getUnderlyingBuffer().size()))))
    , size_(static_cast<int>(parent.getUnderlyingBuffer().size()))
{}

int IndexableByteStream::getAbsoluteIndex() const noexcept
{
    return position_;
}

ca::i64 IndexableByteStream::readableBytesLeft() const noexcept
{
    // 子流返回到 slice_end_ 的剩余量（可为负：position 被 moveTo 越过 slice 时），
    // 顶层流等价于 size_ - position_。供 switch 等按字节预算封顶数量的读取方使用。
    return static_cast<ca::i64>(slice_end_) - position_;
}

int IndexableByteStream::getIndex() const noexcept
{
    return position_ - slice_start_;
}

void IndexableByteStream::moveToAbsolute(int index)
{
    // 敌意 class 可声明越界 attribute content length（如 0x80000000 起的负 int），
    // 负绝对位置一旦通过 require() 的上界检查即产生负下标读（UB）。位置必须非负。
    if (index < 0) {
        throw std::out_of_range("IndexableByteStream: negative absolute position");
    }
    position_ = index;
}

void IndexableByteStream::moveTo(int index)
{
    position_ = index + slice_start_;
}

void IndexableByteStream::moveBack(int distance)
{
    position_ -= distance;
}

void IndexableByteStream::skipBytes(int n)
{
    require(n);
    position_ += n;
}

std::vector<ca::u8> IndexableByteStream::getBuffer() const
{
    if (slice_start_ == 0 && slice_end_ == 0) {
        return *buffer_;
    }
    return std::vector<ca::u8>(buffer_->begin() + slice_start_, buffer_->begin() + slice_end_);
}

const std::vector<ca::u8>& IndexableByteStream::getUnderlyingBuffer() const noexcept
{
    return *buffer_;
}

void IndexableByteStream::require(int n)
{
    if (n < 0) {
        throw std::out_of_range("IndexableByteStream: negative read length");
    }
    if (position_ < 0) {
        throw std::out_of_range("IndexableByteStream: negative stream position");
    }
    // 用减法比较避免 position_ + n 的有符号溢出（敌意长度可达 INT_MAX）。
    if (n > size_ - position_) {
        throw std::out_of_range("IndexableByteStream: read past end of buffer");
    }
    // 注意：子流越界（越过 slice_end_ 但仍在父缓冲区内）刻意保持宽容——
    // crasher 语料中存在内容长度与实际读取量不符的混淆属性，严格拒绝会让
    // 异常在属性解析中途抛出、流停在与内容长度不符的位置，后续属性从错误
    // 偏移级联误解析。宽容语义是本仓对混淆语料的既定契约（对应
    // SliceReadPastSliceEndReadsParentBufferThenThrows）。
}

ca::u8 IndexableByteStream::readUnsignedByte()
{
    require(1);
    return (*buffer_)[position_++];
}

ca::i8 IndexableByteStream::readByte()
{
    return static_cast<ca::i8>(readUnsignedByte());
}

ca::u16 IndexableByteStream::readUnsignedShort()
{
    require(2);
    ca::u16 hi = (*buffer_)[position_++];
    ca::u16 lo = (*buffer_)[position_++];
    return static_cast<ca::u16>((hi << 8) | lo);
}

ca::i16 IndexableByteStream::readShort()
{
    return static_cast<ca::i16>(readUnsignedShort());
}

ca::u32 IndexableByteStream::readInt()
{
    require(4);
    ca::u32 b0 = (*buffer_)[position_++];
    ca::u32 b1 = (*buffer_)[position_++];
    ca::u32 b2 = (*buffer_)[position_++];
    ca::u32 b3 = (*buffer_)[position_++];
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

ca::i32 IndexableByteStream::readRawInt()
{
    return static_cast<ca::i32>(readInt());
}

float IndexableByteStream::readFloat()
{
    ca::u32 raw = readInt();
    float   value;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

ca::u64 IndexableByteStream::readLong()
{
    require(8);
    ca::u64 hi = static_cast<ca::u64>(readInt()) & 0xFFFFFFFFu;
    ca::u64 lo = static_cast<ca::u64>(readInt()) & 0xFFFFFFFFu;
    return (hi << 32) | lo;
}

double IndexableByteStream::readDouble()
{
    ca::u64 raw = readLong();
    double  value;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

std::string IndexableByteStream::readUTF()
{
    return detail::decodeMutf8LikeJava(readMutf8Ref());
}

jstr::MUtf8StringRef IndexableByteStream::readMutf8Ref()
{
    auto len = readUnsignedShort();
    require(len);
    const char* start = reinterpret_cast<const char*>(buffer_->data() + position_);
    position_ += len;
    return jstr::MUtf8StringRef(start, len);
}

void IndexableByteStream::readFully(std::vector<ca::u8>& dest, int n)
{
    require(n);
    dest.insert(dest.end(), buffer_->begin() + position_, buffer_->begin() + position_ + n);
    position_ += n;
}

}   // namespace jclassfile::io
