// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/InstructionWriter.java
// 状态：阶段 5b 实现
// 备注：
//   - writeCode(list) 将 Instruction 列表写回 byte[]。
//   - 关键差异：Java 写 put((byte)(int)) / putShort((short)(int)) / putInt(int)；
//     C++ 对应 put(int) / putShort(int) / putInt(int) 内部已做大端字节序。
//   - 校验：原版每条指令写入后比对 expectedSize vs actual，失败抛 IllegalStateException。
//     C++ → std::logic_error。
//   - fallback 处理：未知 opcode 调 fallbackWriter.write(instruction, buffer)。
//   - 与 reader 配合：reader 解析时给 TABLESWITCH/LOOKUPSWITCH 设了 padding，
//     writer 用同样的 startPos 计算 skip（(4 - (startPos+1) & 3)）保证 round-trip。

#pragma once

#include <memory>
#include <vector>

#include "jclassfile/io/fallback_instruction_writer.hpp"
#include "libca/core/datatype.hpp"
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/util/growing_byte_buffer.hpp"

namespace jclassfile::io {

class InstructionWriter
{
public:
    explicit InstructionWriter(std::unique_ptr<FallbackInstructionWriter> fallback)
        : ownedFallback_(std::move(fallback))
        , fallback_(ownedFallback_.get())
    {
        ensureFallback();
    }

    explicit InstructionWriter(FallbackInstructionWriter* fallback)
        : fallback_(fallback)
    {
        ensureFallback();
    }

    InstructionWriter()
        : InstructionWriter(FallbackInstructionWriter::fail())
    {}

    [[nodiscard]] std::vector<ca::u8> writeCode(const std::vector<const model::Instruction*>& list);

private:
    void ensureFallback()
    {
        if (fallback_ != nullptr) return;
        ownedFallback_ = FallbackInstructionWriter::fail();
        fallback_      = ownedFallback_.get();
    }

    std::unique_ptr<FallbackInstructionWriter> ownedFallback_;
    FallbackInstructionWriter*                 fallback_ = nullptr;
};

}   // namespace jclassfile::io
