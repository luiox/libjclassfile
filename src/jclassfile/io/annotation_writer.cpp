// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/AnnotationWriter.java
// 状态：阶段 6 完整实现
// 备注：
//   - 镜像 AnnotationReader 写路径。
//   - 类型分派使用 kind() 枚举 + static_cast（项目禁用 RTTI）。
//   - writeAnnotation 写 type_index (u2) + num_element_value_pairs (u2) + pairs。
//   - writeTypeAnnotation 追加 target_type (u1) + target_info (??) + type_path (??) +
//     普通 annotation body。
//   - writeElementValue 写 tag (u1) + 按 tag 类型追加对应 payload。

#include "jclassfile/io/annotation_writer.hpp"

#include <stdexcept>

#include "jclassfile/model/annotation/annotation_element_value.hpp"
#include "jclassfile/model/annotation/array_element_value.hpp"
#include "jclassfile/model/annotation/class_element_value.hpp"
#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/annotation/enum_element_value.hpp"
#include "jclassfile/model/annotation/primitive_element_value.hpp"
#include "jclassfile/model/annotation/target_info.hpp"
#include "jclassfile/model/annotation/target_info_type.hpp"
#include "jclassfile/model/annotation/type_annotation.hpp"
#include "jclassfile/model/annotation/type_path.hpp"
#include "jclassfile/model/annotation/type_path_element.hpp"
#include "jclassfile/model/annotation/type_path_kind.hpp"
#include "jclassfile/model/annotation/utf8_element_value.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::io {

using namespace jclassfile::model;

void AnnotationWriter::writeAnnotationDefault(const AnnotationDefaultAttribute& attr)
{
    writeElementValue(*attr.elementValue());
}

void AnnotationWriter::writeAnnotations(const AnnotationsAttribute& annos)
{
    out_.putShort(static_cast<int>(annos.annotations().size()));
    for (const auto& annotation : annos.annotations()) {
        writeAnnotation(*annotation);
    }
}

void AnnotationWriter::writeTypeAnnotations(const AnnotationsAttribute& annos)
{
    out_.putShort(static_cast<int>(annos.annotations().size()));
    for (const auto& annotation : annos.annotations()) {
        // 原版用 (TypeAnnotation) annotation 强转；C++ 用 kind() 枚举校验。
        if (annotation->kind() != AnnotationKind::TypeAnnotation) {
            throw std::logic_error("writeTypeAnnotations called on non-TypeAnnotation entry");
        }
        writeTypeAnnotation(static_cast<const TypeAnnotation&>(*annotation));
    }
}

void AnnotationWriter::writeParameterAnnotations(const ParameterAnnotationsAttribute& annos)
{
    const auto& all = annos.parameterAnnotations();
    out_.put(static_cast<int>(all.size()));
    for (const auto& list : all) {
        out_.putShort(static_cast<int>(list.size()));
        for (const auto& annotation : list) {
            writeAnnotation(*annotation);
        }
    }
}

void AnnotationWriter::writeAnnotation(const Annotation& annotation)
{
    out_.putShort(annotation.getType().index());
    writeElementPairs(annotation);
}

void AnnotationWriter::writeTypeAnnotation(const TypeAnnotation& annotation)
{
    const TargetInfo* info = annotation.getTargetInfo();
    if (info == nullptr) {
        throw std::invalid_argument("TypeAnnotation has no targetInfo");
    }
    out_.put(info->getTargetType());
    switch (info->kind()) {
    case TargetInfoKind::TypeParameter:
    {
        const auto& t = static_cast<const TypeParameterTargetInfo&>(*info);
        out_.put(t.getTypeParameterIndex());
        break;
    }
    case TargetInfoKind::SuperType:
    {
        const auto& t = static_cast<const SuperTypeTargetInfo&>(*info);
        out_.putShort(t.getSuperTypeIndex());
        break;
    }
    case TargetInfoKind::TypeParameterBound:
    {
        const auto& t = static_cast<const TypeParameterBoundTargetInfo&>(*info);
        out_.put(t.getTypeParameterIndex());
        out_.put(t.getBoundIndex());
        break;
    }
    case TargetInfoKind::Empty:
        // no-op
        break;
    case TargetInfoKind::FormalParameter:
    {
        const auto& t = static_cast<const FormalParameterTargetInfo&>(*info);
        out_.put(t.getFormalParameterIndex());
        break;
    }
    case TargetInfoKind::Throws:
    {
        const auto& t = static_cast<const ThrowsTargetInfo&>(*info);
        out_.putShort(t.getThrowsTypeIndex());
        break;
    }
    case TargetInfoKind::LocalVar:
    {
        const auto& t = static_cast<const LocalVarTargetInfo&>(*info);
        out_.putShort(static_cast<int>(t.getVariableTable().size()));
        for (const auto& v : t.getVariableTable()) {
            out_.putShort(v.startPc);
            out_.putShort(v.length);
            out_.putShort(v.index);
        }
        break;
    }
    case TargetInfoKind::Catch:
    {
        const auto& t = static_cast<const CatchTargetInfo&>(*info);
        out_.putShort(t.getExceptionTableIndex());
        break;
    }
    case TargetInfoKind::Offset:
    {
        const auto& t = static_cast<const OffsetTargetInfo&>(*info);
        out_.putShort(t.getOffset());
        break;
    }
    case TargetInfoKind::TypeArgument:
    {
        const auto& t = static_cast<const TypeArgumentTargetInfo&>(*info);
        out_.putShort(t.getOffset());
        out_.put(t.getTypeArgumentIndex());
        break;
    }
    }
    writeTypePath(*annotation.getTypePath());
    writeAnnotation(annotation);
}

void AnnotationWriter::writeTypePath(const TypePath& typePath)
{
    out_.put(static_cast<int>(typePath.getPath().size()));
    for (const auto& element : typePath.getPath()) {
        out_.put(typePathKindValue(element.getKind()));
        out_.put(element.getArgIndex());
    }
}

void AnnotationWriter::writeElementPairs(const Annotation& annotation)
{
    const auto& values = annotation.getValues();
    out_.putShort(static_cast<int>(values.size()));
    for (const auto& [name, value] : values) {
        if (name == nullptr || value == nullptr) continue;
        out_.putShort(name->index());
        writeElementValue(*value);
    }
}

void AnnotationWriter::writeElementValue(const ElementValue& elementValue)
{
    out_.put(static_cast<int>(static_cast<unsigned char>(elementValue.getTag())));
    switch (elementValue.kind()) {
    case ElementValueKind::Utf8:
    {
        const auto& v = static_cast<const Utf8ElementValue&>(elementValue);
        out_.putShort(v.getValue()->index());
        break;
    }
    case ElementValueKind::Primitive:
    {
        const auto&    v = static_cast<const PrimitiveElementValue&>(elementValue);
        const CpEntry* e = v.getValue();
        if (e == nullptr) {
            throw std::invalid_argument("PrimitiveElementValue has null CpEntry");
        }
        out_.putShort(e->index());
        break;
    }
    case ElementValueKind::Enum:
    {
        const auto& v = static_cast<const EnumElementValue&>(elementValue);
        out_.putShort(v.getType()->index());
        out_.putShort(v.getName()->index());
        break;
    }
    case ElementValueKind::Class:
    {
        const auto& v = static_cast<const ClassElementValue&>(elementValue);
        out_.putShort(v.getClassEntry()->index());
        break;
    }
    case ElementValueKind::Annotation:
    {
        const auto& v = static_cast<const AnnotationElementValue&>(elementValue);
        writeAnnotation(*v.getAnnotation());
        break;
    }
    case ElementValueKind::Array:
    {
        const auto& v = static_cast<const ArrayElementValue&>(elementValue);
        out_.putShort(static_cast<int>(v.getArray().size()));
        for (const auto& arrayValue : v.getArray()) {
            if (arrayValue == nullptr) continue;
            writeElementValue(*arrayValue);
        }
        break;
    }
    }
}

}   // namespace jclassfile::io
