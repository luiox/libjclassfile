// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/FallbackInstructionReader.java
// 状态：阶段 5a 实现（header-only 抽象类 + fail() 静态工厂）
// 备注：
//   - Java `interface` + lambda 静态工厂；C++ 等价为 abstract class + 私有 FailFast 子类。
//   - 返回类型 `List<Instruction>` → `std::vector<std::unique_ptr<Instruction>>`（持有所有权）。
//   - 异常 `IllegalStateException` → `std::logic_error`（表达"调用了不可恢复的代码路径"）。
//   - 阶段 5b 由 InstructionReader 实际调用 read() 返回的 vector。
//   - 嵌套类 FailFast 用前向声明 + 外部定义：因为 FailFast 继承 FallbackInstructionReader，
//     而 FallbackInstructionReader 必须先完成定义才能被继承；前向声明在类体内，完整定义在
//     类体外，确保基类已 complete。

#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "indexable_byte_stream.hpp"
#include "jclassfile/model/instruction/instruction.hpp"

namespace jclassfile::io {

class FallbackInstructionReader
{
public:
    virtual ~FallbackInstructionReader() = default;

    [[nodiscard]] virtual std::vector<std::unique_ptr<model::Instruction>> read(
        int opcode, IndexableByteStream& is) = 0;

    [[nodiscard]] static std::unique_ptr<FallbackInstructionReader> fail();

private:
    class FailFast;
};

class FallbackInstructionReader::FailFast final : public FallbackInstructionReader
{
public:
    [[nodiscard]] std::vector<std::unique_ptr<model::Instruction>> read(
        int opcode, IndexableByteStream& /*is*/) override
    {
        throw std::logic_error("Unable to read instruction, opcode=" + std::to_string(opcode));
    }
};

inline std::unique_ptr<FallbackInstructionReader> FallbackInstructionReader::fail()
{
    return std::unique_ptr<FallbackInstructionReader>(new FailFast());
}

}   // namespace jclassfile::io
