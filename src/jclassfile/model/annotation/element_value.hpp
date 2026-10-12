// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/ElementValue.java
// 状态：阶段 6 完整实现
// 备注：
//   - 原版 sealed abstract permits 6 子类。
//   - 项目禁用 RTTI：基类提供 virtual ElementValueKind kind() = 0，
//     调用方通过 kind() 枚举做类型分派 + static_cast 还原子类（与 InsnKind 模式一致）。
//   - getTag() 返回 char（原版 ASCII tag：B/C/D/F/I/J/S/Z/s/e/c/@/[）。
//   - 子类：Utf8ElementValue / PrimitiveElementValue / EnumElementValue /
//     ClassElementValue / AnnotationElementValue / ArrayElementValue。

#pragma once

#include <vector>

#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

enum class ElementValueKind
{
    Utf8,
    Primitive,
    Enum,
    Class,
    Annotation,
    Array,
};

class ElementValue : public behavior::CpAccessor
{
public:
    virtual ~ElementValue() = default;

    ElementValue(const ElementValue&)            = delete;
    ElementValue& operator=(const ElementValue&) = delete;

    [[nodiscard]] virtual ElementValueKind kind() const noexcept = 0;

    [[nodiscard]] char getTag() const noexcept { return tag_; }

    [[nodiscard]] virtual std::vector<const CpEntry*> cpAccesses() const             = 0;
    [[nodiscard]] virtual int                         computeLength() const noexcept = 0;

    explicit ElementValue(char tag) noexcept
        : tag_(tag)
    {}

private:
    char tag_;
};

}   // namespace jclassfile::model
