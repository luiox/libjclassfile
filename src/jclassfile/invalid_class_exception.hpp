// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/InvalidClassException.java
// 状态：阶段 4 实现
// 备注：原版是 checked Exception，两个构造（String / Throwable）。
//       C++ 项目无 checked 概念（除非显式 throw / noexcept 声明），按 std::exception 继承。

#pragma once

#include <exception>
#include <string>
#include <utility>

namespace jclassfile {

class InvalidClassException : public std::exception
{
public:
    explicit InvalidClassException(std::string msg)
        : message_(std::move(msg))
    {}

    explicit InvalidClassException(const std::exception& cause) noexcept
        : message_(cause.what())
    {}

    [[nodiscard]] const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

}   // namespace jclassfile
