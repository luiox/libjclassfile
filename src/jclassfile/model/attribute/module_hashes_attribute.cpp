// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/ModuleHashesAttribute.java

#include "jclassfile/model/attribute/module_hashes_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ModuleHashesAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(algorithmName_);
    for (const auto& kv : moduleHashes_) set.push_back(kv.first);
    return set;
}

int ModuleHashesAttribute::computeInternalLength() const noexcept
{
    // u2: platformName (algorithm name)
    // u2: entry count
    // per entry: u2: name_index, u2: hash_length, N: hash content
    int len = 4;
    for (const auto& kv : moduleHashes_) {
        len += 4 + static_cast<int>(kv.second.size());
    }
    return len;
}

}   // namespace jclassfile::model
