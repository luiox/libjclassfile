// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/LocalVariableTableAttribute.java

#include "jclassfile/model/attribute/local_variable_table_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> LocalVariableTableAttribute::VarEntry::cpAccesses() const
{
    return {name_, desc_};
}

std::vector<const CpEntry*> LocalVariableTableAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const VarEntry& e : entries_) {
        for (const CpEntry* ce : e.cpAccesses()) set.push_back(ce);
    }
    return set;
}

}   // namespace jclassfile::model
