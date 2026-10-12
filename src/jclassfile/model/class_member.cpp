// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/ClassMember.java
// 状态：阶段 4 实现
// 备注：equals 采用指针/序列等价，与 vector operator== 语义一致。
//       语义级 equals（按 attribute 字段逐项比对）需要为 38 个 attribute
//       子类实现 equals，超出阶段 4 范围。Java 原版使用 List.equals + Object.equals，
//       但 CAFED00D 测试中按对象同一性比对是常见做法（同一份 attribute 树）。

#include "jclassfile/model/class_member.hpp"

namespace jclassfile::model {

std::vector<const CpEntry*> ClassMember::cpAccesses() const
{
    std::vector<const CpEntry*> result;
    result.reserve(2 + attributes_.size() * 2);
    result.push_back(name_);
    result.push_back(type_);
    for (const Attribute* attr : attributes_) {
        auto sub = attr->cpAccesses();
        result.insert(result.end(), sub.begin(), sub.end());
    }
    return result;
}

bool ClassMember::equals(const ClassMember& other) const noexcept
{
    if (this == &other) return true;
    if (access_ != other.access_) return false;
    if (attributes_ != other.attributes_) return false;
    return name_ == other.name_ && type_ == other.type_;
}

}   // namespace jclassfile::model
