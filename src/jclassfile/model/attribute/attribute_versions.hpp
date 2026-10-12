// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/AttributeVersions.java
// 状态：阶段 3 实现
// 备注：返回 attribute 引入版本（class file major version）。原版 switch 表达式逐 case yield。
//       未知 attribute 返回 -1。

#pragma once

#include <string_view>

#include "jclassfile/model/attribute/attribute_constants.hpp"
#include "jclassfile/model/attribute/version_constants.hpp"

namespace jclassfile::model {

class AttributeVersions : public AttributeConstants, public VersionConstants
{
public:
    [[nodiscard]] static int getIntroducedVersion(std::string_view attributeName);
};

}   // namespace jclassfile::model
