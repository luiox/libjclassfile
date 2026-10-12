// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/TargetInfo.java
// 状态：阶段 6 完整实现
// 备注：基类 cpAccesses 默认返回空集（与原版一致）；子类均无 CP 引用。

#include "jclassfile/model/annotation/target_info.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> TargetInfo::cpAccesses() const
{
    return {};
}

}   // namespace jclassfile::model
