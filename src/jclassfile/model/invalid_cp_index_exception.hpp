// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/InvalidCpIndexException.java

#pragma once

#include <stdexcept>
#include <string>

namespace jclassfile::model {

class ConstPool;

class InvalidCpIndexException : public std::out_of_range
{
public:
    explicit InvalidCpIndexException(const ConstPool& pool, int index)
        : std::out_of_range("Invalid constant pool index: " + std::to_string(index))
        , pool_(&pool)
        , index_(index)
    {}

    [[nodiscard]] const ConstPool& getPool() const noexcept { return *pool_; }
    [[nodiscard]] int              getIndex() const noexcept { return index_; }

    [[nodiscard]] std::string toString() const noexcept
    {
        return "InvalidCpIndexException{pool=" +
               std::to_string(reinterpret_cast<uintptr_t>(pool_)) +
               ", index=" + std::to_string(index_) + "}";
    }

private:
    const ConstPool* pool_;
    int              index_;
};

}   // namespace jclassfile::model
