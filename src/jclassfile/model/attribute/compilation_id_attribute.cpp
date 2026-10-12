// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/CompilationIdAttribute.java

#include "jclassfile/model/attribute/compilation_id_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> CompilationIdAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(compilationId_);
    return set;
}

}   // namespace jclassfile::model
