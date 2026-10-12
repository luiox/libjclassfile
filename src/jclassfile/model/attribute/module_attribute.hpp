// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModuleAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_module.hpp"
#include "jclassfile/model/constant/cp_package.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class ModuleAttribute : public Attribute
{
public:
    class Requires : public behavior::CpAccessor
    {
    public:
        Requires(const CpModule* module, int flags, const CpUtf8* version) noexcept
            : module_(module)
            , flags_(flags)
            , version_(version)
        {}

        [[nodiscard]] const CpModule* module() const noexcept { return module_; }
        void setModule(const CpModule* module) noexcept { module_ = module; }

        [[nodiscard]] int flags() const noexcept { return flags_; }
        void              setFlags(int flags) noexcept { flags_ = flags; }

        [[nodiscard]] const CpUtf8* version() const noexcept { return version_; }
        void setVersion(const CpUtf8* version) noexcept { version_ = version; }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    private:
        const CpModule* module_;
        int             flags_;
        const CpUtf8*   version_;
    };

    class Exports : public behavior::CpAccessor
    {
    public:
        Exports(const CpPackage* packageEntry, int flags, std::vector<const CpModule*> to)
            : packageEntry_(packageEntry)
            , flags_(flags)
            , to_(std::move(to))
        {}

        [[nodiscard]] const CpPackage* packageEntry() const noexcept { return packageEntry_; }
        void                           setPackageEntry(const CpPackage* packageEntry) noexcept
        {
            packageEntry_ = packageEntry;
        }

        [[nodiscard]] int flags() const noexcept { return flags_; }
        void              setFlags(int flags) noexcept { flags_ = flags; }

        [[nodiscard]] const std::vector<const CpModule*>& to() const noexcept { return to_; }
        void setTo(std::vector<const CpModule*> toIndex) noexcept { to_ = std::move(toIndex); }

        [[nodiscard]] int length() const noexcept
        {
            // 6 = index + flags + list.size()
            return 6 + 2 * static_cast<int>(to_.size());
        }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    private:
        const CpPackage*             packageEntry_;
        int                          flags_;
        std::vector<const CpModule*> to_;
    };

    class Opens : public behavior::CpAccessor
    {
    public:
        Opens(const CpPackage* packageEntry, int flags, std::vector<const CpModule*> to)
            : packageEntry_(packageEntry)
            , flags_(flags)
            , to_(std::move(to))
        {}

        [[nodiscard]] const CpPackage* packageEntry() const noexcept { return packageEntry_; }
        void                           setPackageEntry(const CpPackage* packageEntry) noexcept
        {
            packageEntry_ = packageEntry;
        }

        [[nodiscard]] int flags() const noexcept { return flags_; }
        void              setFlags(int flags) noexcept { flags_ = flags; }

        [[nodiscard]] const std::vector<const CpModule*>& to() const noexcept { return to_; }
        void setTo(std::vector<const CpModule*> toIndex) noexcept { to_ = std::move(toIndex); }

        [[nodiscard]] int length() const noexcept { return 6 + 2 * static_cast<int>(to_.size()); }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    private:
        const CpPackage*             packageEntry_;
        int                          flags_;
        std::vector<const CpModule*> to_;
    };

    class Provides : public behavior::CpAccessor
    {
    public:
        Provides(const CpClass* module, std::vector<const CpClass*> with)
            : module_(module)
            , with_(std::move(with))
        {}

        [[nodiscard]] const CpClass* module() const noexcept { return module_; }
        void                         setModule(const CpClass* module) noexcept { module_ = module; }

        [[nodiscard]] const std::vector<const CpClass*>& with() const noexcept { return with_; }
        void setWith(std::vector<const CpClass*> with) noexcept { with_ = std::move(with); }

        [[nodiscard]] int length() const noexcept
        {
            // 4 = index + list.size()
            return 4 + 2 * static_cast<int>(with_.size());
        }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    private:
        const CpClass*              module_;
        std::vector<const CpClass*> with_;
    };

    ModuleAttribute(const CpUtf8* name, const CpModule* module, int flags, const CpUtf8* version,
                    std::vector<Requires> requires, std::vector<Exports> exports,
                    std::vector<Opens> opens, std::vector<const CpClass*> uses,
                    std::vector<Provides> provides)
        : Attribute(name)
        , module_(module)
        , flags_(flags)
        , version_(version)
        , requires_(std::move(requires))
        , exports_(std::move(exports))
        , opens_(std::move(opens))
        , uses_(std::move(uses))
        , provides_(std::move(provides))
    {}

    [[nodiscard]] const CpModule* module() const noexcept { return module_; }
    void                          setModule(const CpModule* module) noexcept { module_ = module; }

    [[nodiscard]] int flags() const noexcept { return flags_; }
    void              setFlags(int flags) noexcept { flags_ = flags; }

    [[nodiscard]] const CpUtf8* version() const noexcept { return version_; }
    void                        setVersion(const CpUtf8* version) noexcept { version_ = version; }

    [[nodiscard]] const std::vector<Requires>&
        requires()
    const noexcept
    {
        return requires_;
    }
    void setRequires(std::vector<Requires> requires) noexcept { requires_ = std::move(requires); }

    [[nodiscard]] const std::vector<Exports>& exports() const noexcept { return exports_; }
    void setExports(std::vector<Exports> exports) noexcept { exports_ = std::move(exports); }

    [[nodiscard]] const std::vector<Opens>& opens() const noexcept { return opens_; }
    void setOpens(std::vector<Opens> opens) noexcept { opens_ = std::move(opens); }

    [[nodiscard]] const std::vector<const CpClass*>& uses() const noexcept { return uses_; }
    void setUses(std::vector<const CpClass*> uses) noexcept { uses_ = std::move(uses); }

    [[nodiscard]] const std::vector<Provides>& provides() const noexcept { return provides_; }
    void setProvides(std::vector<Provides> provides) noexcept { provides_ = std::move(provides); }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override;

private:
    const CpModule*             module_;
    int                         flags_;
    const CpUtf8*               version_;
    std::vector<Requires>       requires_;
    std::vector<Exports>        exports_;
    std::vector<Opens>          opens_;
    std::vector<const CpClass*> uses_;
    std::vector<Provides>       provides_;
};

}   // namespace jclassfile::model
