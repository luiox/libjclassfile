// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/ClassMember.java
// 状态：阶段 4 实现
// 备注：原版 sealed abstract permits Field, Method。
//       C++ protected 构造 + 纯虚 holderType() 表达 sealed abstract 语义。
//       派生类只能是 Field / Method（与原版 Java permits 语义一致）。
//       原版 getAttribute(Class<T>) 依赖 RTTI；C++ 走 AttributeHolder 基类提供的
//       永远返回 nullptr 的模板（参见 model/behavior/attribute_holder.hpp 注释）。
//       cpAccesses() 行为：name + type + 所有 attribute 的 cpAccesses。
//       equals/hashCode：与原版逐字段对齐。

#pragma once

#include <vector>

#include "jclassfile/model/accessible.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/behavior/attribute_holder.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class ClassMember : public Accessible, public AttributeHolder, public behavior::CpAccessor
{
public:
    ClassMember(std::vector<const Attribute*> attributes, int access, const CpUtf8* name,
                const CpUtf8* type) noexcept
        : attributes_(std::move(attributes))
        , access_(access)
        , name_(name)
        , type_(type)
    {}

    ClassMember(const ClassMember&)            = delete;
    ClassMember& operator=(const ClassMember&) = delete;

    [[nodiscard]] int getAccess() const noexcept override { return access_; }
    void              setAccess(int access) noexcept override { access_ = access; }

    [[nodiscard]] const CpUtf8* name() const noexcept { return name_; }
    void                        setName(const CpUtf8* name) noexcept { name_ = name; }

    [[nodiscard]] const CpUtf8* type() const noexcept { return type_; }
    void                        setType(const CpUtf8* type) noexcept { type_ = type; }

    [[nodiscard]] std::vector<const Attribute*>& attributes() noexcept override
    {
        return attributes_;
    }
    [[nodiscard]] const std::vector<const Attribute*>& attributes() const noexcept override
    {
        return attributes_;
    }
    void setAttributes(std::vector<const Attribute*> attributes) noexcept override
    {
        attributes_ = std::move(attributes);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    [[nodiscard]] bool equals(const ClassMember& other) const noexcept;

protected:
    ClassMember() = default;

private:
    std::vector<const Attribute*> attributes_;
    int                           access_ = 0;
    const CpUtf8*                 name_   = nullptr;
    const CpUtf8*                 type_   = nullptr;
};

}   // namespace jclassfile::model
