// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModuleResolutionAttribute.java

#pragma once

#include "jclassfile/model/attribute/attribute.hpp"

namespace jclassfile::model {

class ModuleResolutionAttribute : public Attribute
{
public:
    static constexpr int DO_NOT_RESOLVE_BY_DEFAULT   = 0x0001;
    static constexpr int WARN_DEPRECATED             = 0x0002;
    static constexpr int WARN_DEPRECATED_FOR_REMOVAL = 0x0004;
    static constexpr int WARN_INCUBATING             = 0x0008;

    ModuleResolutionAttribute(const CpUtf8* name, int flags) noexcept
        : Attribute(name)
        , flags_(flags)
    {}

    [[nodiscard]] int flags() const noexcept { return flags_; }
    void              setFlags(int flags) noexcept { flags_ = flags; }

    [[nodiscard]] int computeInternalLength() const noexcept override { return 2; }

private:
    int flags_;
};

}   // namespace jclassfile::model
