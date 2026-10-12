// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/behavior/CpAccessor.java
// 状态：阶段 2/3 演进
// 备注：原版是含 cpAccesses() 方法的接口。
//       阶段 2 因 CpEntry 派生类需要 cpAccesses()，而 vector<const CpEntry*> 的签名
//       需 CpEntry 完整；当时为避免循环 include 简化为 marker。
//       阶段 3 CpAccessor 加入虚 cpAccesses()。因 cp_entry.hpp 已 include cp_accessor.hpp，
//       cp_accessor.hpp 不能反向 include cp_entry.hpp；改为 forward declaration。
//       std::vector<const CpEntry*> 仅在签名出现即可，vector<pointer> 的 sizeof 固定为
//       3 个指针，与 CpEntry 完整与否无关。

#pragma once

#include <vector>

namespace jclassfile::model {

class CpEntry;

namespace behavior {

class CpAccessor
{
public:
    virtual ~CpAccessor() = default;

    [[nodiscard]] virtual std::vector<const CpEntry*> cpAccesses() const = 0;
};

}   // namespace behavior
}   // namespace jclassfile::model
