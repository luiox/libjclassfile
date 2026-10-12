// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/Method.java
// 状态：阶段 4 实现

#include "jclassfile/model/method.hpp"

namespace jclassfile::model {

namespace {

constexpr int ACC_NATIVE   = 0x0100;
constexpr int ACC_ABSTRACT = 0x0400;

}   // namespace

std::vector<const CpEntry*> Method::cpAccesses() const
{
    std::vector<const CpEntry*> result;
    result.reserve(2 + attributes().size() * 2);
    result.push_back(name());
    result.push_back(type());

    const int  accessMask       = getAccess();
    const bool nativeOrAbstract = (accessMask & (ACC_NATIVE | ACC_ABSTRACT)) != 0;

    for (const Attribute* attr : attributes()) {
        if (attr->isCodeAttribute() && nativeOrAbstract) {
            // 原版 Java 在此处 logger.warn("Code attribute found on native or abstract method: {}",
            // this); 项目无 logger，保留跳过行为。
            continue;
        }
        auto sub = attr->cpAccesses();
        result.insert(result.end(), sub.begin(), sub.end());
    }
    return result;
}

std::string Method::toString() const
{
    std::string out;
    out.reserve(64);
    if (name() != nullptr) out += name()->text();
    if (type() != nullptr) out += type()->text();
    return out;
}

}   // namespace jclassfile::model
