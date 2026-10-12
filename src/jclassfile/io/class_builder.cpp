// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/ClassBuilder.java
// 状态：阶段 4b 实现

#include "class_builder.hpp"

namespace jclassfile::io {

model::ClassFile ClassBuilder::build()
{
    model::ClassFile::ModelStorage storage;
    storage.fields       = std::move(fieldStorage_);
    storage.methods      = std::move(methodStorage_);
    storage.attributes   = std::move(attributeStorage_);
    storage.instructions = std::move(instructionStorage_);
    return model::ClassFile(versionMinor_,
                            versionMajor_,
                            std::move(pool_),
                            access_,
                            thisClass_,
                            superClass_,
                            interfaces_,
                            fields_,
                            methods_,
                            attributes_,
                            std::move(storage));
}

}   // namespace jclassfile::io
