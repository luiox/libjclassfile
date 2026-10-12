// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/LocalVariableTableAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class LocalVariableTableAttribute : public Attribute
{
public:
    class VarEntry : public behavior::CpAccessor
    {
    public:
        VarEntry(int startPc, int length, const CpUtf8* name, const CpUtf8* desc,
                 int index) noexcept
            : startPc_(startPc)
            , length_(length)
            , name_(name)
            , desc_(desc)
            , index_(index)
        {}

        [[nodiscard]] int           startPc() const noexcept { return startPc_; }
        [[nodiscard]] int           length() const noexcept { return length_; }
        [[nodiscard]] const CpUtf8* name() const noexcept { return name_; }
        [[nodiscard]] const CpUtf8* desc() const noexcept { return desc_; }
        [[nodiscard]] int           index() const noexcept { return index_; }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    private:
        int           startPc_;
        int           length_;
        const CpUtf8* name_;
        const CpUtf8* desc_;
        int           index_;
    };

    LocalVariableTableAttribute(const CpUtf8* name, std::vector<VarEntry> entries) noexcept
        : Attribute(name)
        , entries_(std::move(entries))
    {}

    [[nodiscard]] const std::vector<VarEntry>& entries() const noexcept { return entries_; }
    void setEntries(std::vector<VarEntry> entries) noexcept { entries_ = std::move(entries); }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override
    {
        return 2 + static_cast<int>(entries_.size()) * 10;
    }

private:
    std::vector<VarEntry> entries_;
};

}   // namespace jclassfile::model
