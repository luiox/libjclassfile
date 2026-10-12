// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/SourceFileAttribute.java

#include "jclassfile/model/attribute/source_file_attribute.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> SourceFileAttribute::cpAccesses() const
{
    auto set = Attribute::cpAccesses();
    set.push_back(sourceFilename_);
    return set;
}

}   // namespace jclassfile::model
