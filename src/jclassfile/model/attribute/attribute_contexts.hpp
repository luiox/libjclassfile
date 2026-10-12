// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/AttributeContexts.java
// 状态：阶段 3 实现
// 备注：返回 attribute 名对应的合法位置集合。Java EnumSet<AttributeHolderType>
//       → C++ std::vector<AttributeHolderType>，调用方通过排序+比较判断包含。
//       原版的 default 行为（未知 attribute 允许所有位置）保留。

#pragma once

#include <string_view>
#include <vector>

#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/model/attribute/attribute_constants.hpp"

namespace jclassfile::model {

class AttributeContexts : public AttributeConstants
{
public:
    [[nodiscard]] static std::vector<jclassfile::io::AttributeHolderType> getAllowedContexts(
        std::string_view attributeName);

private:
    static std::vector<jclassfile::io::AttributeHolderType> enumAll();
    static std::vector<jclassfile::io::AttributeHolderType> enumClass();
    static std::vector<jclassfile::io::AttributeHolderType> enumField();
    static std::vector<jclassfile::io::AttributeHolderType> enumMethod();
    static std::vector<jclassfile::io::AttributeHolderType> enumClassFieldMethod();
    static std::vector<jclassfile::io::AttributeHolderType> enumAttribute();
};

}   // namespace jclassfile::model
