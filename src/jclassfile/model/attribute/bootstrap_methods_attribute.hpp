// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/BootstrapMethodsAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_method_handle.hpp"

namespace jclassfile::model {

class BootstrapMethodsAttribute : public Attribute
{
public:
    class BootstrapMethod : public behavior::CpAccessor
    {
    public:
        BootstrapMethod(const CpMethodHandle*       bsmMethodRef,
                        std::vector<const CpEntry*> args) noexcept
            : bsmMethodRef_(bsmMethodRef)
            , args_(std::move(args))
        {}

        [[nodiscard]] const CpMethodHandle* bsmMethodRef() const noexcept { return bsmMethodRef_; }
        void setBsmMethodRef(const CpMethodHandle* bsmMethodRef) noexcept
        {
            bsmMethodRef_ = bsmMethodRef;
        }

        [[nodiscard]] const std::vector<const CpEntry*>& args() const noexcept { return args_; }
        void setArgs(std::vector<const CpEntry*> args) noexcept { args_ = std::move(args); }

        [[nodiscard]] int computeLength() const noexcept
        {
            // u2: bootstrap_method_ref
            // u2: num_bootstrap_arguments
            // u2 * args: bootstrap_arguments[]
            return 4 + static_cast<int>(args_.size()) * 2;
        }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    private:
        const CpMethodHandle*       bsmMethodRef_;
        std::vector<const CpEntry*> args_;
    };

    BootstrapMethodsAttribute(const CpUtf8* name, std::vector<BootstrapMethod> bootstrapMethods)
        : Attribute(name)
        , bootstrapMethods_(std::move(bootstrapMethods))
    {}

    [[nodiscard]] const std::vector<BootstrapMethod>& bootstrapMethods() const noexcept
    {
        return bootstrapMethods_;
    }
    void setBootstrapMethods(std::vector<BootstrapMethod> bootstrapMethods) noexcept
    {
        bootstrapMethods_ = std::move(bootstrapMethods);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override
    {
        int len = 2;
        for (const BootstrapMethod& bsm : bootstrapMethods_) len += bsm.computeLength();
        return len;
    }

private:
    std::vector<BootstrapMethod> bootstrapMethods_;
};

}   // namespace jclassfile::model
