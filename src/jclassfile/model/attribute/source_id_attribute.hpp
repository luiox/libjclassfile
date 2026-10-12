// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/SourceIdAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class SourceIdAttribute : public Attribute
{
public:
    SourceIdAttribute(const CpUtf8* name, const CpUtf8* sourceId) noexcept
        : Attribute(name)
        , sourceId_(sourceId)
    {}

    [[nodiscard]] const CpUtf8* sourceId() const noexcept { return sourceId_; }
    void setSourceId(const CpUtf8* sourceId) noexcept { sourceId_ = sourceId; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int computeInternalLength() const noexcept override { return 2; }

private:
    const CpUtf8* sourceId_;
};

}   // namespace jclassfile::model
