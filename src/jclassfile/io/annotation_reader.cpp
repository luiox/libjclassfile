#include "jclassfile/io/annotation_reader.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "jclassfile/io/class_builder.hpp"
#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/model/annotation/annotation_element_value.hpp"
#include "jclassfile/model/annotation/array_element_value.hpp"
#include "jclassfile/model/annotation/class_element_value.hpp"
#include "jclassfile/model/annotation/element_value_constants.hpp"
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
#include "jclassfile/model/descriptor.hpp"
#include "jclassfile/transform/illegal_stripping_transformer.hpp"

namespace jclassfile::io {

using namespace jclassfile::model;

namespace {

const CpUtf8* asCpUtf8(const CpEntry* e)
{
    if (e == nullptr || e->tag() != CpTag::UTF8) return nullptr;
    return static_cast<const CpUtf8*>(e);
}

void rethrowIfStrict(const ClassFileReader& reader)
{
    if (!reader.doDropEofAttributes()) {
        throw;
    }
}

/// 枚举类型的 type_name 必须是对象（L...;）字段描述符（JVMS 4.7.16.1，
/// 上游 f19d7129 的 AnnotationReader#isValidEnumTypeDescriptor）。
bool isValidEnumTypeDescriptor(const std::string& text)
{
    return Descriptor::from(text).getKind() == Descriptor::Kind::Object;
}

}   // namespace

int AnnotationReader::requiredConstantTagForPrimitive(char tag)
{
    // 上游 PrimitiveElementValue.requiredConstantTag（JVMS 4.7.16.1-A）。
    switch (tag) {
    case 'B':
    case 'C':
    case 'I':
    case 'S':
    case 'Z': return static_cast<int>(CpTag::INTEGER);
    case 'F': return static_cast<int>(CpTag::FLOAT);
    case 'J': return static_cast<int>(CpTag::LONG);
    case 'D': return static_cast<int>(CpTag::DOUBLE);
    default: return -1;
    }
}

AnnotationReader::AnnotationReader(ClassFileReader& reader, ClassBuilder& builder,
                                   IndexableByteStream& is, int length, const CpUtf8* name,
                                   AttributeContext context, bool visible)
    : reader_(reader)
    , builder_(builder)
    , cp_(builder.getPool())
    , parentIs_(is)
    , length_(length)
    , name_(name)
    , context_(context)
    , maxCpIndex_(cp_.size())
    , visible_(visible)
{}

// ---- public reads ----

AnnotationDefaultAttribute* AnnotationReader::readAnnotationDefault()
{
    try {
        AnnotationScope scope;
        auto            ev = readElementValue(scope);
        if (ev == nullptr) return nullptr;
        return builder_.createAttribute<AnnotationDefaultAttribute>(name_, std::move(ev));
    }
    catch (const std::exception&) {
        rethrowIfStrict(reader_);
        return nullptr;
    }
}

AnnotationsAttribute* AnnotationReader::readAnnotations()
{
    try {
        int numAnnotations = parentIs_.readUnsignedShort();
        if (numAnnotations == 0) {
            return nullptr;
        }

        std::unordered_set<std::string>          usedAnnotationTypes;
        std::vector<std::unique_ptr<Annotation>> annotations;
        annotations.reserve(static_cast<std::size_t>(numAnnotations));
        for (int i = 0; i < numAnnotations; i++) {
            try {
                AnnotationScope scope;
                auto            annotation = readAnnotation(scope);
                if (annotation == nullptr) continue;
                if (reader_.doDropDupeAnnotations()) {
                    std::string type = annotation->getType().text();
                    if (usedAnnotationTypes.find(type) == usedAnnotationTypes.end()) {
                        usedAnnotationTypes.insert(type);
                        annotations.push_back(std::move(annotation));
                    }
                }
                else {
                    annotations.push_back(std::move(annotation));
                }
            }
            catch (const std::exception&) {
                // 单个 annotation 读失败后，剩余字节的边界无法再可靠判断。整个属性
                // 交给外层宽容分支丢弃，避免输出被错误截断的半个注解属性。
                throw;
            }
        }

        if (annotations.empty()) return nullptr;
        return builder_.createAttribute<AnnotationsAttribute>(name_, std::move(annotations), visible_);
    }
    catch (const std::exception&) {
        rethrowIfStrict(reader_);
        return nullptr;
    }
}

ParameterAnnotationsAttribute* AnnotationReader::readParameterAnnotations()
{
    try {
        int numParameters = parentIs_.readUnsignedByte();
        if (numParameters == 0) {
            return nullptr;
        }

        std::vector<std::vector<std::unique_ptr<Annotation>>> parameterAnnotations;
        parameterAnnotations.reserve(static_cast<std::size_t>(numParameters));
        for (int p = 0; p < numParameters; p++) {
            int numParamAnnotations = parentIs_.readUnsignedShort();
            std::vector<std::unique_ptr<Annotation>> annotations;
            annotations.reserve(static_cast<std::size_t>(numParamAnnotations));
            for (int i = 0; i < numParamAnnotations; i++) {
                AnnotationScope scope;
                annotations.push_back(readAnnotation(scope));
            }
            parameterAnnotations.push_back(std::move(annotations));
        }
        return builder_.createAttribute<ParameterAnnotationsAttribute>(name_, std::move(parameterAnnotations), visible_);
    }
    catch (const std::exception&) {
        rethrowIfStrict(reader_);
        return nullptr;
    }
}

AnnotationsAttribute* AnnotationReader::readTypeAnnotations()
{
    try {
        int numAnnotations = parentIs_.readUnsignedShort();
        if (numAnnotations == 0) {
            return nullptr;
        }

        std::vector<std::unique_ptr<Annotation>> annotations;
        annotations.reserve(static_cast<std::size_t>(numAnnotations));
        // 单条注解被丢后流停在注解中间，后续注解若误解析抛错仍落入外层宽容
        // catch 丢整属性，不会输出截断的半个注解（与 readAnnotations 既有
        // 不变量一致）；全部条目被丢则整属性返回 nullptr（上游 6489374e）。
        int kept = 0;
        for (int i = 0; i < numAnnotations; i++) {
            AnnotationScope scope;
            auto annotation = readTypeAnnotation(scope);
            if (annotation != nullptr) {
                annotations.push_back(std::move(annotation));
                ++kept;
            }
        }
        if (kept == 0) {
            return nullptr;
        }
        return builder_.createAttribute<AnnotationsAttribute>(name_, std::move(annotations), visible_);
    }
    catch (const std::exception&) {
        rethrowIfStrict(reader_);
        return nullptr;
    }
}

// ---- private reads ----

std::unique_ptr<Annotation> AnnotationReader::readAnnotation(AnnotationScope& scope)
{
    int typeIndex = parentIs_.readUnsignedShort();
    if (typeIndex > maxCpIndex_) {
        throw std::invalid_argument("Annotation type_index out of CP bounds: " +
                                    std::to_string(typeIndex));
    }
    const CpEntry* entry = cp_.get(typeIndex);
    const CpUtf8*  type  = asCpUtf8(entry);
    if (type == nullptr) {
        throw std::invalid_argument("Annotation type_index does not point to UTF8: " +
                                    std::to_string(typeIndex));
    }
    auto values = readElementPairs(scope.with(type->text()));
    return std::make_unique<Annotation>(type, std::move(values));
}

std::unique_ptr<TypeAnnotation> AnnotationReader::readTypeAnnotation(AnnotationScope& scope)
{
    int targetType = parentIs_.readUnsignedByte();
    // 校验 target 与 holder 匹配；0x13（FIELD / empty_target）同时允许出现在
    // field 与 record component 声明（JVMS 4.7.20.1，上游 6489374e），未知
    // target 一律拒绝。不匹配按本文件宽容语义丢弃该注解，不再抛异常。
    if (!isAnnotationTargetTypeValidIn(targetType, context_.type())) {
        return nullptr;
    }
    TargetInfoType infoType = targetInfoTypeFromTargetType(targetType);

    std::unique_ptr<TargetInfo> info;
    switch (infoType) {
    case TargetInfoType::TYPE_PARAMETER_TARGET:
    {
        int typeParameterIndex = parentIs_.readUnsignedByte();
        info = std::make_unique<TypeParameterTargetInfo>(targetType, typeParameterIndex);
        break;
    }
    case TargetInfoType::SUPERTYPE_TARGET:
    {
        int superTypeIndex = parentIs_.readUnsignedShort();
        info               = std::make_unique<SuperTypeTargetInfo>(targetType, superTypeIndex);
        break;
    }
    case TargetInfoType::TYPE_PARAMETER_BOUND_TARGET:
    {
        int typeParameterIndex = parentIs_.readUnsignedByte();
        int boundIndex         = parentIs_.readUnsignedByte();
        info                   = std::make_unique<TypeParameterBoundTargetInfo>(
            targetType, typeParameterIndex, boundIndex);
        break;
    }
    case TargetInfoType::EMPTY_TARGET:
    {
        info = std::make_unique<EmptyTargetInfo>(targetType);
        break;
    }
    case TargetInfoType::FORMAL_PARAMETER_TARGET:
    {
        int formalParameterIndex = parentIs_.readUnsignedByte();
        info = std::make_unique<FormalParameterTargetInfo>(targetType, formalParameterIndex);
        break;
    }
    case TargetInfoType::THROWS_TARGET:
    {
        int throwsTypeIndex = parentIs_.readUnsignedShort();
        info                = std::make_unique<ThrowsTargetInfo>(targetType, throwsTypeIndex);
        break;
    }
    case TargetInfoType::LOCALVAR_TARGET:
    {
        int                                       tableLength = parentIs_.readUnsignedShort();
        std::vector<LocalVarTargetInfo::Variable> variables;
        variables.reserve(static_cast<std::size_t>(tableLength));
        for (int i = 0; i < tableLength; i++) {
            int startPc = parentIs_.readUnsignedShort();
            int len     = parentIs_.readUnsignedShort();
            int index   = parentIs_.readUnsignedShort();
            variables.emplace_back(startPc, len, index);
        }
        info = std::make_unique<LocalVarTargetInfo>(targetType, std::move(variables));
        break;
    }
    case TargetInfoType::CATCH_TARGET:
    {
        int exceptionTableIndex = parentIs_.readUnsignedShort();
        info = std::make_unique<CatchTargetInfo>(targetType, exceptionTableIndex);
        break;
    }
    case TargetInfoType::OFFSET_TARGET:
    {
        int offset = parentIs_.readUnsignedShort();
        info       = std::make_unique<OffsetTargetInfo>(targetType, offset);
        break;
    }
    case TargetInfoType::TYPE_ARGUMENT_TARGET:
    {
        int offset            = parentIs_.readUnsignedShort();
        int typeArgumentIndex = parentIs_.readUnsignedByte();
        info = std::make_unique<TypeArgumentTargetInfo>(targetType, offset, typeArgumentIndex);
        break;
    }
    }

    TypePath typePath = readTypePath();

    int typeIndex = parentIs_.readUnsignedShort();
    if (typeIndex > maxCpIndex_) {
        throw std::invalid_argument("Annotation type_index out of CP bounds: " +
                                    std::to_string(typeIndex));
    }
    const CpEntry* entry = cp_.get(typeIndex);
    const CpUtf8*  type  = asCpUtf8(entry);
    if (type == nullptr) {
        throw std::invalid_argument(
            "Annotation type_index does not match allowed locations for its type");
    }
    auto values = readElementPairs(scope.with(type->text()));
    return std::make_unique<TypeAnnotation>(
        type, std::move(values), std::move(info), std::make_unique<TypePath>(std::move(typePath)));
}

TypePath AnnotationReader::readTypePath()
{
    int                          length = parentIs_.readUnsignedByte();
    std::vector<TypePathElement> elements;
    elements.reserve(static_cast<std::size_t>(length));
    for (int i = 0; i < length; i++) {
        int kind  = parentIs_.readUnsignedByte();
        int index = parentIs_.readUnsignedByte();
        elements.emplace_back(typePathKindFromValue(kind), index);
    }
    return TypePath(std::move(elements));
}

std::map<const CpUtf8*, std::unique_ptr<ElementValue>> AnnotationReader::readElementPairs(
    AnnotationScope& scope)
{
    int                                                    numPairs = parentIs_.readUnsignedShort();
    std::map<const CpUtf8*, std::unique_ptr<ElementValue>> values;
    while (numPairs > 0) {
        int            nameIndex    = parentIs_.readUnsignedShort();
        auto           elementValue = readElementValue(scope);
        const CpEntry* nameEntry    = cp_.get(nameIndex);
        const CpUtf8*  elementName  = asCpUtf8(nameEntry);
        if (elementName == nullptr) {
            throw std::invalid_argument("Element pair has invalid name specified by cp-index: " +
                                        std::to_string(nameIndex));
        }
        if (values.find(elementName) != values.end()) {
            throw std::invalid_argument(std::string("Element pairs already has field by name: ") +
                                        elementName->text());
        }
        values.emplace(elementName, std::move(elementValue));
        numPairs--;
    }
    return values;
}

std::unique_ptr<ElementValue> AnnotationReader::readElementValue(AnnotationScope& scope)
{
    // 栈深度限制必须在 readElementValue 入口：纯嵌套数组（tag '[' 数组套数组）的
    // 递归边不经过 readElementPairs，只在 pairs 入口检查会被完全绕过，
    // 几 MB 的敌意 class 可造百万级递归深度打爆栈（外层 catch 接不住栈溢出）。
    // annotation 边（readAnnotation → readElementPairs）同样汇聚到本入口。
    // depth_ 计实际调用栈深度；AnnotationScope 因 with() 可变累积语义会统计
    // 兄弟元素（扁平大数组注解是合法样本），不能用作深度度量。
    if (depth_ >= MAX_NESTING) {
        throw std::invalid_argument("Bogus deep annotation packing detected");
    }
    DepthGuard guard(*this);

    char tag = static_cast<char>(parentIs_.readUnsignedByte());
    switch (tag) {
    case ElementValueConstants::TAG_BYTE:
    case ElementValueConstants::TAG_CHAR:
    case ElementValueConstants::TAG_DOUBLE:
    case ElementValueConstants::TAG_FLOAT:
    case ElementValueConstants::TAG_INT:
    case ElementValueConstants::TAG_LONG:
    case ElementValueConstants::TAG_SHORT:
    case ElementValueConstants::TAG_BOOLEAN:
    {
        int            index = parentIs_.readUnsignedShort();
        const CpEntry* entry = cp_.get(index);
        // 常量池条目种类必须与 tag 声明的原始类型匹配（上游 66d31cf8）。
        // 越界/无法解析的引用仍按本文件既有宽容契约保留（见下），交由
        // IllegalStrippingTransformer 以属性为单位删除；不猜测替代常量池索引。
        if (entry != nullptr &&
            entry->tag() != static_cast<CpTag>(requiredConstantTagForPrimitive(tag))) {
            throw std::invalid_argument(std::string("Invalid element: ") + tag);
        }
        return std::make_unique<PrimitiveElementValue>(tag, entry);
    }
    case ElementValueConstants::TAG_STRING:
    {
        int            utfIndex = parentIs_.readUnsignedShort();
        const CpEntry* entry    = cp_.get(utfIndex);
        const CpUtf8*  utf      = asCpUtf8(entry);
        if (utf == nullptr) {
            throw std::invalid_argument("Invalid string element");
        }
        return std::make_unique<Utf8ElementValue>(tag, utf);
    }
    case ElementValueConstants::TAG_ENUM:
    {
        int            typenameIdx   = parentIs_.readUnsignedShort();
        int            constname     = parentIs_.readUnsignedShort();
        const CpEntry* entryType     = cp_.get(typenameIdx);
        const CpEntry* entryConstant = cp_.get(constname);
        const CpUtf8*  typeUtf       = asCpUtf8(entryType);
        const CpUtf8*  constUtf      = asCpUtf8(entryConstant);
        // type_name 必须是对象字段描述符、const_name 必须是非限定名
        // （JVMS 4.7.16.1，上游 f19d7129）。
        if (typeUtf == nullptr || constUtf == nullptr ||
            !isValidEnumTypeDescriptor(typeUtf->text()) ||
            !transform::IllegalStrippingTransformer::isValidUnqualifiedName(constUtf->text())) {
            throw std::invalid_argument("Invalid element: Enum");
        }
        return std::make_unique<EnumElementValue>(tag, typeUtf, constUtf);
    }
    case ElementValueConstants::TAG_CLASS:
    {
        int            classInfoIndex = parentIs_.readUnsignedShort();
        const CpEntry* entry          = cp_.get(classInfoIndex);
        const CpUtf8*  className      = asCpUtf8(entry);
        if (className == nullptr) {
            throw std::invalid_argument("Invalid class element");
        }
        return std::make_unique<ClassElementValue>(tag, className);
    }
    case ElementValueConstants::TAG_ANNOTATION:
    {
        auto nested = readAnnotation(scope);
        return std::make_unique<AnnotationElementValue>(tag, std::move(nested));
    }
    case ElementValueConstants::TAG_ARRAY:
    {
        int                                        numElements = parentIs_.readUnsignedShort();
        std::vector<std::unique_ptr<ElementValue>> arrayValues;
        arrayValues.reserve(static_cast<std::size_t>(numElements));
        for (int i = 0; i < numElements; i++) {
            arrayValues.push_back(readElementValue(scope.with("[")));
        }
        return std::make_unique<ArrayElementValue>(tag, std::move(arrayValues));
    }
    default:
        throw std::invalid_argument(std::string("Unrecognized tag for annotation element value: ") +
                                    tag);
    }
}

}   // namespace jclassfile::io
