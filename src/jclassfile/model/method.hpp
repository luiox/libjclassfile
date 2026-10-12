// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/Method.java
// 状态：阶段 4 实现
// 备注：原版 non-sealed class extends ClassMember。
//       cpAccesses() 重写：name + type 来自基类；遍历 attribute，若为 CodeAttribute
//       且方法标记 ACC_NATIVE/ACC_ABSTRACT 则跳过（与原版 logger.warn 对齐语义）。
//       C++ 跳过 logger.warn（项目无日志层），保留行为核心。
//       toString() 返回 "name()type"（无空格，方法签名格式）。

#pragma once

#include <string>
#include <vector>

#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/class_member.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class Method : public ClassMember
{
public:
    Method(std::vector<const Attribute*> attributes, int access, const CpUtf8* name,
           const CpUtf8* type) noexcept
        : ClassMember(std::move(attributes), access, name, type)
    {}

    [[nodiscard]] jclassfile::io::AttributeHolderType holderType() const noexcept override
    {
        return jclassfile::io::AttributeHolderType::METHOD;
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    [[nodiscard]] std::string toString() const;
};

}   // namespace jclassfile::model
