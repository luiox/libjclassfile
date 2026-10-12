// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/LineNumberTableAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"

namespace jclassfile::model {

class LineNumberTableAttribute : public Attribute
{
public:
    class LineEntry
    {
    public:
        LineEntry(int startPc, int line) noexcept
            : startPc_(startPc)
            , line_(line)
        {}
        [[nodiscard]] int startPc() const noexcept { return startPc_; }
        [[nodiscard]] int line() const noexcept { return line_; }

    private:
        int startPc_;
        int line_;
    };

    LineNumberTableAttribute(const CpUtf8* name, std::vector<LineEntry> entries) noexcept
        : Attribute(name)
        , entries_(std::move(entries))
    {}

    [[nodiscard]] const std::vector<LineEntry>& entries() const noexcept { return entries_; }
    void setEntries(std::vector<LineEntry> entries) noexcept { entries_ = std::move(entries); }

    [[nodiscard]] int computeInternalLength() const noexcept override
    {
        return 2 + static_cast<int>(entries_.size()) * 4;
    }

private:
    std::vector<LineEntry> entries_;
};

}   // namespace jclassfile::model
