// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/Field.java
// 状态：阶段 4 实现

#include "jclassfile/model/field.hpp"

namespace jclassfile::model {

std::string Field::toString() const
{
    std::string out;
    out.reserve(64);
    if (name() != nullptr) out += name()->text();
    out += ' ';
    if (type() != nullptr) out += type()->text();
    return out;
}

}   // namespace jclassfile::model
