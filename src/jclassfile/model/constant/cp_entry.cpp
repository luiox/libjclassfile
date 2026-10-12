// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpEntry.java
// 状态：阶段 2 实现
// 备注：原版 checkIndices 静态开关在 C++ 中简化为 consteval-free 实现；
//       若需要校验（assert index_ > 0），后续可加 debug-only 检查。

#include "jclassfile/model/constant/cp_entry.hpp"

#include "jclassfile/model/constant/cross_cp_referencing.hpp"

namespace jclassfile::model {

ca::u16 CpEntry::index() const
{
    return index_;
}

void CpEntry::setIndex(ca::u16 index) noexcept
{
    index_ = index;
}

}   // namespace jclassfile::model
