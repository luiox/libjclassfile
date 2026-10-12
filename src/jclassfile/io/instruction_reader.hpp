// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/InstructionReader.java
// 状态：阶段 5b 实现
// 备注：
//   - read(is, pool, codeLength) 走完一段 code 区段，构造所有 Instruction。
//   - opcode 走大 switch 分派到 8 个 Instruction 子类。
//   - TABLESWITCH/LOOKUPSWITCH 在读取 padding 后调 notifyStartPosition(pos - 1)，
//     与原版一致：pos 是 opcode 之后的第一个字节（即 padding 起点），pos - 1 是 opcode 位置。
//   - WIDE 嵌套：backing 指令由 reader 构造并包装为 WideInstruction。
//   - fallback 处理：未知 opcode 调 fallbackReader.read(opcode, is)，
//     阶段 5b 默认 fail-fast（由 ClassFileReader 在阶段 5 注入 fallback）。
//   - 异常：Java 抛 IOException（classfile 读取错误用 InvalidClassException 包装）；
//     C++ 直接抛 std::runtime_error / std::logic_error，不引入 IOException 模拟。

#pragma once

#include <memory>
#include <vector>

#include "jclassfile/io/fallback_instruction_reader.hpp"
#include "jclassfile/io/indexable_byte_stream.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/instruction/instruction.hpp"

namespace jclassfile::io {

class InstructionReader
{
public:
    explicit InstructionReader(std::unique_ptr<FallbackInstructionReader> fallback)
        : fallback_(std::move(fallback))
    {}

    InstructionReader()
        : InstructionReader(FallbackInstructionReader::fail())
    {}

    [[nodiscard]] std::vector<std::unique_ptr<model::Instruction>> read(
        IndexableByteStream& is, const model::ConstPool& pool, int codeLength);

    // 静态 helper（与原版 public static 保持一致）
    [[nodiscard]] static std::unique_ptr<model::Instruction> readXLoad(IndexableByteStream& is,
                                                                       int                  opcode);

    [[nodiscard]] static std::unique_ptr<model::Instruction> readLdc(IndexableByteStream&    is,
                                                                     const model::ConstPool& pool);

    [[nodiscard]] static std::unique_ptr<model::Instruction> readLdcW(IndexableByteStream&    is,
                                                                      const model::ConstPool& pool,
                                                                      int opcode);

    [[nodiscard]] static std::unique_ptr<model::Instruction> readLookupSwitchInstruction(
        IndexableByteStream& is);

    [[nodiscard]] static std::unique_ptr<model::Instruction> readMemberReferenceInstruction(
        IndexableByteStream& is, const model::ConstPool& pool, int opcode);

private:
    std::unique_ptr<FallbackInstructionReader> fallback_;
};

}   // namespace jclassfile::io
