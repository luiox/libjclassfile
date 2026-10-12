// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/ConstRef.java

#include "jclassfile/model/constant/const_ref.hpp"

#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"

namespace jclassfile::model {

ConstRef::ConstRef(CpTag tag, const CpClass* classRef, const CpNameType* nameType)
    : CpEntry(tag)
    , classRef_(classRef)
    , nameType_(nameType)
{}

std::vector<const CpEntry*> ConstRef::cpAccesses() const
{
    return {this, classRef_, nameType_};
}

std::vector<const CpEntry*> ConstRef::getReferences() const
{
    return {classRef_, nameType_};
}

bool ConstRef::equals(const CpEntry& other) const
{
    if (!other.isConstRef()) return false;
    const auto& o = static_cast<const ConstRef&>(other);
    return classRef_->equals(*o.classRef_) && nameType_->equals(*o.nameType_);
}

}   // namespace jclassfile::model
