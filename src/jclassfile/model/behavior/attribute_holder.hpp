// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/behavior/AttributeHolder.java
// 状态：阶段 3 实现（简化版）
// 备注：原版 sealed interface permits
// ClassFile/ClassMember/CodeAttribute/RecordAttribute.RecordComponent。
//       C++ 用 abstract class + protected 构造表达 sealed 语义。
//       原版 <T extends Attribute> T getAttribute(Class<T> type) 依赖 Java RTTI；
//       C++ 禁用 RTTI，getAttribute<T>() 简化为返回 nullptr（基类默认）。
//       唯一使用方 IllegalStrippingTransformer 属于阶段 8 实现范围。

#pragma once

#include <vector>

#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/model/attribute/attribute.hpp"

namespace jclassfile::model {

class AttributeHolder
{
public:
    virtual ~AttributeHolder() = default;

    [[nodiscard]] virtual std::vector<const Attribute*>&       attributes()       = 0;
    [[nodiscard]] virtual const std::vector<const Attribute*>& attributes() const = 0;

    template<typename T>
    [[nodiscard]] T* getAttribute() noexcept
    {
        // 项目禁用 RTTI（无 dynamic_cast / typeid）。阶段 8 IllegalStrippingTransformer
        // 真正需要按类型查询 attribute 时会改用基于 tag/enum 的查表；本接口对所有 holder
        // 一律返回 nullptr。CodeAttribute::getAttribute<T> 走自身维护的缓存路径。
        (void)0;
        return nullptr;
    }

    template<typename T>
    [[nodiscard]] const T* getAttribute() const noexcept
    {
        return nullptr;
    }

    virtual void setAttributes(std::vector<const Attribute*> attributes) = 0;

    [[nodiscard]] virtual jclassfile::io::AttributeHolderType holderType() const = 0;

protected:
    AttributeHolder() = default;
};

}   // namespace jclassfile::model
