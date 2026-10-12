// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpUtf8.java

#include "jclassfile/model/constant/cp_utf8.hpp"

#include "libca/core/datatype.hpp"

namespace jclassfile::model {

CpUtf8::CpUtf8(std::string text)
    : CpEntry(CpTag::UTF8)
    , text_(std::move(text))
{}

void CpUtf8::setText(std::string text)
{
    text_ = std::move(text);
}

std::vector<const CpEntry*> CpUtf8::cpAccesses() const
{
    return {this};
}

bool CpUtf8::equals(const CpEntry& other) const
{
    if (other.tag() != CpTag::UTF8) return false;
    return text_ == static_cast<const CpUtf8&>(other).text_;
}

}   // namespace jclassfile::model
