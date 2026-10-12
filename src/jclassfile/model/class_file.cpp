// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/ClassFile.java
// 状态：阶段 4 实现

#include "jclassfile/model/class_file.hpp"

namespace jclassfile::model {

ClassFile::ClassFile(int versionMinor, int versionMajor, std::unique_ptr<ConstPool> pool,
                     int access, const CpClass* thisClass, const CpClass* superClass,
                     std::vector<const CpClass*> interfaceClasses, std::vector<const Field*> fields,
                     std::vector<const Method*> methods, std::vector<const Attribute*> attributes,
                     ModelStorage storage)
    : pool_(std::move(pool))
    , interfaceClasses_(std::move(interfaceClasses))
    , fields_(std::move(fields))
    , methods_(std::move(methods))
    , attributes_(std::move(attributes))
    , storage_(std::move(storage))
    , access_(access)
    , versionMinor_(versionMinor)
    , versionMajor_(versionMajor)
    , thisClass_(thisClass)
    , superClass_(superClass)
{}

std::string ClassFile::getName() const
{
    if (thisClass_ == nullptr) return {};
    return thisClass_->name()->text();
}

std::string ClassFile::getSuperName() const
{
    if (superClass_ == nullptr) return {};
    return superClass_->name()->text();
}

const CpEntry* ClassFile::getCp(int index) const
{
    return pool_->get(index);
}

std::vector<const CpEntry*> ClassFile::cpAccesses() const
{
    std::vector<const CpEntry*> result;
    result.reserve(8 + attributes_.size() * 2);
    result.push_back(thisClass_);
    if (superClass_ != nullptr) result.push_back(superClass_);
    result.insert(result.end(), interfaceClasses_.begin(), interfaceClasses_.end());
    for (const Attribute* attr : attributes_) {
        auto sub = attr->cpAccesses();
        result.insert(result.end(), sub.begin(), sub.end());
    }
    for (const Field* field : fields_) {
        auto sub = field->cpAccesses();
        result.insert(result.end(), sub.begin(), sub.end());
    }
    for (const Method* method : methods_) {
        auto sub = method->cpAccesses();
        result.insert(result.end(), sub.begin(), sub.end());
    }
    return result;
}

}   // namespace jclassfile::model
