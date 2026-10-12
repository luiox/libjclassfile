// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpMethodHandle.java

#include "jclassfile/model/constant/cp_method_handle.hpp"

#include "jclassfile/model/constant/const_ref.hpp"

namespace jclassfile::model {

CpMethodHandle::CpMethodHandle(ca::u8 kind, const ConstRef* reference)
    : CpEntry(CpTag::METHOD_HANDLE)
    , kind_(kind)
    , reference_(reference)
{}

std::vector<const CpEntry*> CpMethodHandle::cpAccesses() const
{
    return {this, reference_};
}

std::vector<const CpEntry*> CpMethodHandle::getReferences() const
{
    return {reference_};
}

bool CpMethodHandle::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::METHOD_HANDLE) return false;
    const auto& o = static_cast<const CpMethodHandle&>(other);
    return kind_ == o.kind_ && reference_ == o.reference_;
}

}   // namespace jclassfile::model
