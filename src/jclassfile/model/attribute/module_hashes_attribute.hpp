// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModuleHashesAttribute.java

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class ModuleHashesAttribute : public Attribute
{
public:
    ModuleHashesAttribute(const CpUtf8* name, const CpUtf8* algorithmName,
                          std::unordered_map<const CpUtf8*, std::vector<ca::u8>> moduleHashes)
        : Attribute(name)
        , algorithmName_(algorithmName)
        , moduleHashes_(std::move(moduleHashes))
    {}

    [[nodiscard]] const CpUtf8* algorithmName() const noexcept { return algorithmName_; }
    void setAlgorithmName(const CpUtf8* algorithmName) noexcept { algorithmName_ = algorithmName; }

    [[nodiscard]] const std::unordered_map<const CpUtf8*, std::vector<ca::u8>>&
    moduleHashes() const noexcept
    {
        return moduleHashes_;
    }
    void setModuleHashes(
        std::unordered_map<const CpUtf8*, std::vector<ca::u8>> moduleHashes) noexcept
    {
        moduleHashes_ = std::move(moduleHashes);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override;

private:
    const CpUtf8*                                          algorithmName_;
    std::unordered_map<const CpUtf8*, std::vector<ca::u8>> moduleHashes_;
};

}   // namespace jclassfile::model
