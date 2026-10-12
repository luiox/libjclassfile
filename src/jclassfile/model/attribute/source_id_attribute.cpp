// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/SourceIdAttribute.java

#include "jclassfile/model/attribute/source_id_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> SourceIdAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(sourceId_);
    return set;
}

}   // namespace jclassfile::model
