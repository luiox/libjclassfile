// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/util/GrowingByteBuffer.java
// 状态：阶段 5a 实现（1:1 移植 JVM-aligned write 语义）
// 备注：
//   - Java 用 java.nio.ByteBuffer + flip()/put() 实现"按需扩容"语义。
//   - C++ 用 std::vector<ca::u8> + 手动 position_ 游标实现等效行为：
//     * put*(value) 写入 1/2/4/8 字节（big-endian，对齐 JVM classfile 字节序）
//     * position() 返回已写入字节数
//     * ensureWriteable(N) 按 (N+16)>>1<<1 向上取偶扩容
//     * skip(N) 等价于 position += N（Java ByteBuffer.position(pos)）
//   - 不做 ByteBuffer.flip() 模式切换：vector 模型 position_ 即写入游标，扩容时
//     std::vector::resize 保留已写入数据，零开销。

#pragma once

#include <cstddef>
#include <cstring>
#include <vector>

#include "libca/core/datatype.hpp"

namespace jclassfile::util {

class GrowingByteBuffer
{
public:
    GrowingByteBuffer() { buffer_.reserve(1024); }

    GrowingByteBuffer& ensureWriteable(int size)
    {
        int newLength = position_ + size;
        int capacity  = static_cast<int>(buffer_.size());
        if (capacity < newLength) {
            int newCapacity = (newLength + 16) >> 1 << 1;
            buffer_.resize(static_cast<std::size_t>(newCapacity));
        }
        return *this;
    }

    GrowingByteBuffer& put(int value)
    {
        ensureWriteable(1);
        buffer_[static_cast<std::size_t>(position_++)] = static_cast<ca::u8>(value & 0xFF);
        return *this;
    }

    GrowingByteBuffer& putShort(int value)
    {
        ensureWriteable(2);
        ca::u16 v                                      = static_cast<ca::u16>(value & 0xFFFF);
        buffer_[static_cast<std::size_t>(position_++)] = static_cast<ca::u8>((v >> 8) & 0xFF);
        buffer_[static_cast<std::size_t>(position_++)] = static_cast<ca::u8>(v & 0xFF);
        return *this;
    }

    GrowingByteBuffer& putInt(int value)
    {
        ensureWriteable(4);
        ca::u32 v                                      = static_cast<ca::u32>(value);
        buffer_[static_cast<std::size_t>(position_++)] = static_cast<ca::u8>((v >> 24) & 0xFF);
        buffer_[static_cast<std::size_t>(position_++)] = static_cast<ca::u8>((v >> 16) & 0xFF);
        buffer_[static_cast<std::size_t>(position_++)] = static_cast<ca::u8>((v >> 8) & 0xFF);
        buffer_[static_cast<std::size_t>(position_++)] = static_cast<ca::u8>(v & 0xFF);
        return *this;
    }

    GrowingByteBuffer& putLong(long long value)
    {
        ensureWriteable(8);
        ca::u64 v = static_cast<ca::u64>(value);
        for (int i = 7; i >= 0; --i) {
            buffer_[static_cast<std::size_t>(position_++)] =
                static_cast<ca::u8>((v >> (i * 8)) & 0xFF);
        }
        return *this;
    }

    GrowingByteBuffer& putFloat(float value)
    {
        ca::u32 bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        return putInt(static_cast<int>(bits));
    }

    GrowingByteBuffer& putDouble(double value)
    {
        ca::u64 bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        return putLong(static_cast<long long>(bits));
    }

    [[nodiscard]] int position() const noexcept { return position_; }

    GrowingByteBuffer& skip(int bytes)
    {
        ensureWriteable(bytes);
        position_ += bytes;
        return *this;
    }

    void trim() noexcept { buffer_.resize(static_cast<std::size_t>(position_)); }

    [[nodiscard]] const std::vector<ca::u8>& unwrap() const noexcept { return buffer_; }

    std::vector<ca::u8>& unwrap() noexcept { return buffer_; }

private:
    std::vector<ca::u8> buffer_;
    int                 position_ = 0;
};

}   // namespace jclassfile::util
