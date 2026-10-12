// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/CompilationIdAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class CompilationIdAttribute : public Attribute
{
public:
    CompilationIdAttribute(const CpUtf8* name, const CpUtf8* compilationId) noexcept
        : Attribute(name)
        , compilationId_(compilationId)
    {}

    [[nodiscard]] const CpUtf8* compilationId() const noexcept { return compilationId_; }
    void setCompilationId(const CpUtf8* compilationId) noexcept { compilationId_ = compilationId; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int computeInternalLength() const noexcept override { return 2; }

private:
    const CpUtf8* compilationId_;
};

}   // namespace jclassfile::model
