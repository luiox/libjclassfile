// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/SourceFileAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class SourceFileAttribute : public Attribute
{
public:
    SourceFileAttribute(const CpUtf8* name, const CpUtf8* sourceFilename) noexcept
        : Attribute(name)
        , sourceFilename_(sourceFilename)
    {}

    [[nodiscard]] const CpUtf8* sourceFilename() const noexcept { return sourceFilename_; }
    void                        setSourceFilename(const CpUtf8* sourceFilename) noexcept
    {
        sourceFilename_ = sourceFilename;
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int computeInternalLength() const noexcept override { return 2; }

private:
    const CpUtf8* sourceFilename_;
};

}   // namespace jclassfile::model
