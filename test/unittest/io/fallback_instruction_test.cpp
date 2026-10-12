// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/FallbackInstructionReader.java
// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/FallbackInstructionWriter.java
// 覆盖：
//   - fail() 工厂返回非空 unique_ptr
//   - 调用 read(opcode, is) 抛 std::logic_error（含 opcode 信息）
//   - 调用 write(instruction, buffer) 抛 std::logic_error（含 instruction.getOpcode() 信息）

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "jclassfile/io/fallback_instruction_reader.hpp"
#include "jclassfile/io/fallback_instruction_writer.hpp"
#include "jclassfile/io/indexable_byte_stream.hpp"
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/util/growing_byte_buffer.hpp"

using jclassfile::io::FallbackInstructionReader;
using jclassfile::io::FallbackInstructionWriter;
using jclassfile::io::IndexableByteStream;
using jclassfile::util::GrowingByteBuffer;

namespace {

// 阶段 5a：Instruction 仍是 stage 3 stub（仅 getOpcode/computeSize 纯虚）。
// 测试需要一个具体 Instruction，最小定义一个 DummyInstruction。
class DummyInstruction final : public jclassfile::model::Instruction
{
public:
    explicit DummyInstruction(int opcode)
        : Instruction(opcode)
        , opcode_(opcode)
    {}
    [[nodiscard]] int                         getOpcode() const noexcept { return opcode_; }
    [[nodiscard]] int                         computeSize() const noexcept override { return 1; }
    [[nodiscard]] jclassfile::model::InsnKind kind() const noexcept override
    {
        return jclassfile::model::InsnKind::Basic;
    }

private:
    int opcode_;
};

}   // namespace

TEST(FallbackInstructionReaderTest, FailReturnsNonNull)
{
    auto reader = FallbackInstructionReader::fail();
    EXPECT_NE(reader, nullptr);
}

TEST(FallbackInstructionReaderTest, ReadThrowsWithOpcode)
{
    auto                reader = FallbackInstructionReader::fail();
    std::vector<ca::u8> data   = {0x01, 0x02, 0x03};
    IndexableByteStream is(data);

    try {
        (void)reader->read(0xAB, is);
        FAIL() << "expected std::logic_error";
    }
    catch (const std::logic_error& e) {
        std::string what = e.what();
        // 原版用 "+ opcode" 拼接，整数 toString 默认十进制；与 Java 行为一致。
        EXPECT_NE(what.find("171"), std::string::npos)
            << "what() should mention opcode; got: " << what;
        EXPECT_NE(what.find("Unable to read instruction"), std::string::npos);
    }
}

TEST(FallbackInstructionWriterTest, FailReturnsNonNull)
{
    auto writer = FallbackInstructionWriter::fail();
    EXPECT_NE(writer, nullptr);
}

TEST(FallbackInstructionWriterTest, WriteThrowsWithInstructionOpcode)
{
    auto              writer = FallbackInstructionWriter::fail();
    DummyInstruction  ins(0xCD);
    GrowingByteBuffer buf;

    try {
        writer->write(ins, buf);
        FAIL() << "expected std::logic_error";
    }
    catch (const std::logic_error& e) {
        std::string what = e.what();
        // 原版 lambda 用 "+ opcode" 拼接，整数默认十进制。
        EXPECT_NE(what.find("205"), std::string::npos)
            << "what() should mention opcode; got: " << what;
        // 原版 lambda 笔误保留："Unable to writer: "（少一个 'e'）
        EXPECT_NE(what.find("Unable to writer"), std::string::npos);
    }
}

TEST(FallbackInstructionWriterTest, WriteDoesNotMutateBuffer)
{
    auto              writer = FallbackInstructionWriter::fail();
    DummyInstruction  ins(0xFF);
    GrowingByteBuffer buf;
    buf.put(0x42);

    try {
        writer->write(ins, buf);
    }
    catch (const std::logic_error&) {
        // expected
    }
    EXPECT_EQ(buf.position(), 1);
    EXPECT_EQ(buf.unwrap()[0], 0x42u);
}
