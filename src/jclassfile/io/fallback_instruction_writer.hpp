// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/FallbackInstructionWriter.java
// 状态：阶段 5a 实现（header-only 抽象类 + fail() 静态工厂）
// 备注：
//   - Java `interface` + lambda 静态工厂；C++ 等价为 abstract class + 私有 FailFast 子类。
//   - write 参数为 `Instruction`（值传递，Java 引用语义）→ C++ `const Instruction&`。
//   - 异常 `IllegalStateException` → `std::logic_error`。
//   - 注意：原版 lambda 写的是 "Unable to writer:"（typo），与原版 1:1 保留。
//   - 嵌套类 FailFast 模式同 FallbackInstructionReader：前向声明 + 外部定义。

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/util/growing_byte_buffer.hpp"

namespace jclassfile::io {

class FallbackInstructionWriter
{
public:
    virtual ~FallbackInstructionWriter() = default;

    virtual void write(const model::Instruction& instruction, util::GrowingByteBuffer& buffer) = 0;

    [[nodiscard]] static std::unique_ptr<FallbackInstructionWriter> fail();

private:
    class FailFast;
};

class FallbackInstructionWriter::FailFast final : public FallbackInstructionWriter
{
public:
    void write(const model::Instruction& instruction, util::GrowingByteBuffer& /*buffer*/) override
    {
        throw std::logic_error("Unable to writer: " + std::to_string(instruction.getOpcode()));
    }
};

inline std::unique_ptr<FallbackInstructionWriter> FallbackInstructionWriter::fail()
{
    return std::unique_ptr<FallbackInstructionWriter>(new FailFast());
}

}   // namespace jclassfile::io
