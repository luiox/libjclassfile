// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/ConstDynamic.java

#include "jclassfile/model/constant/const_dynamic.hpp"

#include "jclassfile/model/constant/cp_name_type.hpp"

namespace jclassfile::model {

ConstDynamic::ConstDynamic(CpTag tag, ca::u16 bsmIndex, const CpNameType* nameType)
    : CpEntry(tag)
    , bsmIndex_(bsmIndex)
    , nameType_(nameType)
{}

std::vector<const CpEntry*> ConstDynamic::cpAccesses() const
{
    return {this, nameType_};
}

std::vector<const CpEntry*> ConstDynamic::getReferences() const
{
    return {nameType_};
}

bool ConstDynamic::equals(const CpEntry& other) const
{
    if (!other.isConstDynamic()) return false;
    const auto& o = static_cast<const ConstDynamic&>(other);
    return bsmIndex_ == o.bsmIndex_ && nameType_ == o.nameType_;
}

}   // namespace jclassfile::model
