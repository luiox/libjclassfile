// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/SignatureAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class SignatureAttribute : public Attribute
{
public:
    SignatureAttribute(const CpUtf8* name, const CpUtf8* signature) noexcept
        : Attribute(name)
        , signature_(signature)
    {}

    [[nodiscard]] const CpUtf8* signature() const noexcept { return signature_; }
    void setSignature(const CpUtf8* signature) noexcept { signature_ = signature; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int computeInternalLength() const noexcept override { return 2; }

private:
    const CpUtf8* signature_;
};

}   // namespace jclassfile::model
