// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/AnnotationWriter.java
// 状态：阶段 6 完整实现
// 备注：
//   - 原版持 DataOutputStream；C++ 用 util::GrowingByteBuffer（与 InstructionWriter 一致）。
//   - 类型分派：原版用 instanceof + 强转；C++ 用 ElementValue::kind() + static_cast；
//     写 TargetInfo 用 TargetInfo::kind() + static_cast。
//   - 写出的 CpEntry 通过 getIndex() 拿 CP 索引（与原版 Java 一致，
//     Java 的 getIndex() 在 CpEntry 基类已定义，C++ 的 CpEntry 也提供）。
//   - 与 reader 对称：readAnnotation/readTypeAnnotation/readElementValue 镜像
//     writeAnnotation/writeTypeAnnotation/writeElementValue。
//   - AnnotationDefaultAttribute / AnnotationsAttribute / ParameterAnnotationsAttribute
//     三个 attribute 类的 write 入口。

#pragma once

#include "jclassfile/model/annotation/type_annotation.hpp"
#include "jclassfile/model/annotation/type_path.hpp"
#include "jclassfile/model/attribute/annotation_default_attribute.hpp"
#include "jclassfile/model/attribute/annotations_attribute.hpp"
#include "jclassfile/model/attribute/parameter_annotations_attribute.hpp"
#include "jclassfile/util/growing_byte_buffer.hpp"

namespace jclassfile::io {

class AnnotationWriter
{
public:
    explicit AnnotationWriter(util::GrowingByteBuffer& out)
        : out_(out)
    {}

    void writeAnnotationDefault(const model::AnnotationDefaultAttribute& attr);
    void writeAnnotations(const model::AnnotationsAttribute& annos);
    void writeTypeAnnotations(const model::AnnotationsAttribute& annos);
    void writeParameterAnnotations(const model::ParameterAnnotationsAttribute& annos);

private:
    void writeAnnotation(const model::Annotation& annotation);
    void writeTypeAnnotation(const model::TypeAnnotation& annotation);
    void writeTypePath(const model::TypePath& typePath);
    void writeElementPairs(const model::Annotation& annotation);
    void writeElementValue(const model::ElementValue& elementValue);

    util::GrowingByteBuffer& out_;
};

}   // namespace jclassfile::io
