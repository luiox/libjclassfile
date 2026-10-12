// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/Annotation.java
// 状态：阶段 6 完整实现
// 备注：
//   - 原版持有 type(CpUtf8) + values(Map<CpUtf8, ElementValue>)。
//   - C++ 用 std::map 保留插入顺序（与 LinkedHashMap 行为一致）。
//   - AnnotationKind 枚举：TypeAnnotation 是 Annotation 的子类，
//     故基类 kind() 默认返回 Annotation；TypeAnnotation 重写为 TypeAnnotation。
//   - 与 ElementValue 不同，Annotation 不被 sealed；TypeAnnotation extends Annotation。
//     C++ 表达：Annotation 抽象？原版 Annotation 是具体类，允许直接实例化。
//     这里 C++ 保持非 abstract（与原版一致），cpAccesses / computeLength 已是 final 行为。

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

enum class AnnotationKind
{
    Annotation,
    TypeAnnotation,
};

class Annotation : public behavior::CpAccessor
{
public:
    virtual ~Annotation() = default;

    Annotation(const Annotation&)            = delete;
    Annotation& operator=(const Annotation&) = delete;

    [[nodiscard]] virtual AnnotationKind kind() const noexcept
    {
        return AnnotationKind::Annotation;
    }

    [[nodiscard]] const CpUtf8& getType() const noexcept { return *type_; }
    [[nodiscard]] const CpUtf8* typePtr() const noexcept { return type_; }

    [[nodiscard]] const std::map<const CpUtf8*, std::unique_ptr<ElementValue>>&
    getValues() const noexcept
    {
        return values_;
    }
    std::map<const CpUtf8*, std::unique_ptr<ElementValue>>& getValues() noexcept { return values_; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] virtual int                 computeLength() const noexcept;

public:
    Annotation(const CpUtf8* type, std::map<const CpUtf8*, std::unique_ptr<ElementValue>> values)
        : type_(type)
        , values_(std::move(values))
    {}

    // 测试 stub 用的便捷默认构造（与原版 Java 自由构造一致）。
    Annotation() noexcept
        : type_(nullptr)
    {}

private:
    const CpUtf8*                                          type_;
    std::map<const CpUtf8*, std::unique_ptr<ElementValue>> values_;
};

}   // namespace jclassfile::model
