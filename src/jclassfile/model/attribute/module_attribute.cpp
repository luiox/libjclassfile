// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModuleAttribute.java

#include "jclassfile/model/attribute/module_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ModuleAttribute::Requires::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    if (version_ != nullptr) set.push_back(version_);
    set.push_back(module_);
    return set;
}

std::vector<const CpEntry*> ModuleAttribute::Exports::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    set.push_back(packageEntry_);
    for (const CpModule* m : to_) set.push_back(m);
    return set;
}

std::vector<const CpEntry*> ModuleAttribute::Opens::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    set.push_back(packageEntry_);
    for (const CpModule* m : to_) set.push_back(m);
    return set;
}

std::vector<const CpEntry*> ModuleAttribute::Provides::cpAccesses() const
{
    std::vector<const CpEntry*> set;
    set.push_back(module_);
    for (const CpClass* c : with_) set.push_back(c);
    return set;
}

std::vector<const CpEntry*> ModuleAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(module_);
    if (version_ != nullptr) set.push_back(version_);
    for (const CpClass* c : uses_) set.push_back(c);
    for (const Requires& r : requires_) {
        for (const CpEntry* e : r.cpAccesses()) set.push_back(e);
    }
    for (const Exports& e : exports_) {
        for (const CpEntry* ce : e.cpAccesses()) set.push_back(ce);
    }
    for (const Opens& o : opens_) {
        for (const CpEntry* ce : o.cpAccesses()) set.push_back(ce);
    }
    for (const Provides& p : provides_) {
        for (const CpEntry* ce : p.cpAccesses()) set.push_back(ce);
    }
    return set;
}

int ModuleAttribute::computeInternalLength() const noexcept
{
    // 6 = module_name_index + module_flags + module_version_index
    int len = 6;
    // requires = count + requires(u2 * 3)
    len += 2 + static_cast<int>(requires_.size()) * 6;
    // exports
    len += 2;
    for (const Exports& e : exports_) len += e.length();
    // opens
    len += 2;
    for (const Opens& o : opens_) len += o.length();
    // uses = uses_count + list[u2]
    len += 2 + static_cast<int>(uses_.size()) * 2;
    // provides
    len += 2;
    for (const Provides& p : provides_) len += p.length();
    return len;
}

}   // namespace jclassfile::model
