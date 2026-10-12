// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/Field.java
// 状态：阶段 4 实现
// 备注：原版 non-sealed class extends ClassMember。
//       C++ 公开构造（non-sealed equivalent）。
//       holderType() 固定为 FIELD。
//       toString() 返回 "name type"，便于日志。

#pragma once

#include <string>
#include <vector>

#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/class_member.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class Field : public ClassMember
{
public:
    Field(std::vector<const Attribute*> attributes, int access, const CpUtf8* name,
          const CpUtf8* type) noexcept
        : ClassMember(std::move(attributes), access, name, type)
    {}

    [[nodiscard]] jclassfile::io::AttributeHolderType holderType() const noexcept override
    {
        return jclassfile::io::AttributeHolderType::FIELD;
    }

    [[nodiscard]] std::string toString() const;
};

}   // namespace jclassfile::model
