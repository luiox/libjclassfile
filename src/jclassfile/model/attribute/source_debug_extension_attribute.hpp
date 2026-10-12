// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/SourceDebugExtensionAttribute.java

#pragma once

#include <cstdint>
#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"

namespace jclassfile::model {

class SourceDebugExtensionAttribute : public Attribute
{
public:
    SourceDebugExtensionAttribute(const CpUtf8* name, std::vector<ca::u8> debugExtension) noexcept
        : Attribute(name)
        , debugExtension_(std::move(debugExtension))
    {}

    [[nodiscard]] const std::vector<ca::u8>& debugExtension() const noexcept
    {
        return debugExtension_;
    }
    void setDebugExtension(std::vector<ca::u8> debugExtension) noexcept
    {
        debugExtension_ = std::move(debugExtension);
    }

    [[nodiscard]] int computeInternalLength() const noexcept override
    {
        return static_cast<int>(debugExtension_.size());
    }

private:
    std::vector<ca::u8> debugExtension_;
};

}   // namespace jclassfile::model
