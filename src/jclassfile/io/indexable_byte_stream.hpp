// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/IndexableByteStream.java
// 状态：阶段 1 实现（行为对齐 + 单元测试）
// 备注：CAFED00D 基于 java.io.DataInputStream + 自定义 ByteArrayInputStream 子类。
//       C++ 用 std::vector<u8> + 手动边界检查实现 read* 方法；readUTF 保留 modified UTF-8 行为。
//       slice 语义与原版完全一致：slice_start_ = parent.position_，slice_end_ =
//       min(parent.position_ + length, buffer.size())。

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "libca/core/datatype.hpp"
#include "jstr/mutf8_string.hpp"

namespace jclassfile::io {

class IndexableByteStream
{
public:
    explicit IndexableByteStream(const std::vector<ca::u8>& data);

    IndexableByteStream(const IndexableByteStream& parent, int length);

    [[nodiscard]] int getAbsoluteIndex() const noexcept;
    [[nodiscard]] int getIndex() const noexcept;

    /// @brief 返回当前位置到子流上界（slice_end_）的剩余可读字节数。
    ///
    /// 顶层流等价于 size_ - position_；供按字节预算封顶读取数量的调用方
    /// （switch 指令项数等）使用，position 被 moveTo 越过 slice 时可为负。
    [[nodiscard]] ca::i64 readableBytesLeft() const noexcept;

    void moveToAbsolute(int index);
    void moveTo(int index);
    void moveBack(int distance);
    void skipBytes(int n);

    [[nodiscard]] std::vector<ca::u8>        getBuffer() const;
    [[nodiscard]] const std::vector<ca::u8>& getUnderlyingBuffer() const noexcept;

    [[nodiscard]] ca::u8                        readUnsignedByte();
    ca::i8                                      readByte();
    ca::u16                                     readUnsignedShort();
    ca::i16                                     readShort();
    ca::u32                                     readInt();
    ca::i32                                     readRawInt();
    float                                       readFloat();
    ca::u64                                     readLong();
    double                                      readDouble();
    std::string                                 readUTF();
    [[nodiscard]] jstr::MUtf8StringRef readMutf8Ref();
    void                                        readFully(std::vector<ca::u8>& dest, int n);

private:
    const std::vector<ca::u8>* buffer_;
    int                        position_;
    int                        slice_start_;
    int                        slice_end_;
    int                        size_;

    void require(int n);
};

}   // namespace jclassfile::io
