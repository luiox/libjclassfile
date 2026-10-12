// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/LocalVariableTypeTableAttribute.java

#include "jclassfile/model/attribute/local_variable_type_table_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> LocalVariableTypeTableAttribute::VarTypeEntry::cpAccesses() const
{
    return {name_, signature_};
}

std::vector<const CpEntry*> LocalVariableTypeTableAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    for (const VarTypeEntry& e : entries_) {
        for (const CpEntry* ce : e.cpAccesses()) set.push_back(ce);
    }
    return set;
}

}   // namespace jclassfile::model
