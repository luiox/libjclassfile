// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/transform/Transformer.java

#pragma once

#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"

namespace jclassfile::transform {

class Transformer
{
public:
    Transformer(model::ClassFile& clazz)
        : clazz_(clazz)
        , pool_(clazz.getPool())
    {}

    virtual ~Transformer() = default;

    virtual void transform() = 0;

protected:
    model::ClassFile&       clazz_;
    const model::ConstPool& pool_;
};

}   // namespace jclassfile::transform
