#include "jclassfile/io/attribute_reader.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "jclassfile/io/annotation_reader.hpp"
#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/instruction_reader.hpp"
#include "libca/core/datatype.hpp"
#include "jclassfile/model/annotation/target_info.hpp"
#include "jclassfile/model/attribute/annotation_default_attribute.hpp"
#include "jclassfile/model/attribute/annotations_attribute.hpp"
#include "jclassfile/model/attribute/attribute_constants.hpp"
#include "jclassfile/model/attribute/attribute_contexts.hpp"
#include "jclassfile/model/attribute/attribute_versions.hpp"
#include "jclassfile/model/attribute/bootstrap_methods_attribute.hpp"
#include "jclassfile/model/attribute/character_range_table_attribute.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/compilation_id_attribute.hpp"
#include "jclassfile/model/attribute/constant_value_attribute.hpp"
#include "jclassfile/model/attribute/deprecated_attribute.hpp"
#include "jclassfile/model/attribute/enclosing_method_attribute.hpp"
#include "jclassfile/model/attribute/exceptions_attribute.hpp"
#include "jclassfile/model/attribute/inner_classes_attribute.hpp"
#include "jclassfile/model/attribute/line_number_table_attribute.hpp"
#include "jclassfile/model/attribute/local_variable_table_attribute.hpp"
#include "jclassfile/model/attribute/local_variable_type_table_attribute.hpp"
#include "jclassfile/model/attribute/method_parameters_attribute.hpp"
#include "jclassfile/model/attribute/module_attribute.hpp"
#include "jclassfile/model/attribute/module_hashes_attribute.hpp"
#include "jclassfile/model/attribute/module_main_class_attribute.hpp"
#include "jclassfile/model/attribute/module_packages_attribute.hpp"
#include "jclassfile/model/attribute/module_resolution_attribute.hpp"
#include "jclassfile/model/attribute/module_target_attribute.hpp"
#include "jclassfile/model/attribute/nest_host_attribute.hpp"
#include "jclassfile/model/attribute/nest_members_attribute.hpp"
#include "jclassfile/model/attribute/parameter_annotations_attribute.hpp"
#include "jclassfile/model/attribute/permitted_classes_attribute.hpp"
#include "jclassfile/model/attribute/record_attribute.hpp"
#include "jclassfile/model/attribute/signature_attribute.hpp"
#include "jclassfile/model/attribute/source_debug_extension_attribute.hpp"
#include "jclassfile/model/attribute/source_file_attribute.hpp"
#include "jclassfile/model/attribute/source_id_attribute.hpp"
#include "jclassfile/model/attribute/stack_map_table_attribute.hpp"
#include "jclassfile/model/attribute/stack_map_table_constants.hpp"
#include "jclassfile/model/attribute/synthetic_attribute.hpp"
#include "jclassfile/model/attribute/version_constants.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_method_handle.hpp"
#include "jclassfile/model/constant/cp_module.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
#include "jclassfile/model/constant/cp_package.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/constant/placeholders.hpp"
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/transform/illegal_stripping_transformer.hpp"

namespace jclassfile::io {

using namespace jclassfile::model;

// ---- Cp lookup helpers ----

const CpUtf8* AttributeReader::asUtf8OrNull(const ConstPool& cp, int index)
{
    if (index <= 0) return nullptr;
    const CpEntry* e = cp.get(index);
    if (e == nullptr || e->tag() != CpTag::UTF8) return nullptr;
    return static_cast<const CpUtf8*>(e);
}

const CpClass* AttributeReader::asClassOrNull(const ConstPool& cp, int index)
{
    if (index <= 0) return nullptr;
    const CpEntry* e = cp.get(index);
    if (e == nullptr || e->tag() != CpTag::CLASS) return nullptr;
    return static_cast<const CpClass*>(e);
}

const CpNameType* AttributeReader::asNameTypeOrNull(const ConstPool& cp, int index)
{
    if (index <= 0) return nullptr;
    const CpEntry* e = cp.get(index);
    if (e == nullptr || e->tag() != CpTag::NAME_TYPE) return nullptr;
    return static_cast<const CpNameType*>(e);
}

const CpModule* AttributeReader::asModuleOrNull(const ConstPool& cp, int index)
{
    if (index <= 0) return nullptr;
    const CpEntry* e = cp.get(index);
    if (e == nullptr || e->tag() != CpTag::MODULE) return nullptr;
    return static_cast<const CpModule*>(e);
}

const CpPackage* AttributeReader::asPackageOrNull(const ConstPool& cp, int index)
{
    if (index <= 0) return nullptr;
    const CpEntry* e = cp.get(index);
    if (e == nullptr || e->tag() != CpTag::PACKAGE) return nullptr;
    return static_cast<const CpPackage*>(e);
}

// ---- Constructor ----

AttributeReader::AttributeReader(ClassFileReader& reader, ClassBuilder& builder,
                                 IndexableByteStream&& is, int length, const CpUtf8* name,
                                 std::string attrName)
    : reader_(reader)
    , builder_(builder)
    , cp_(builder.getPool())
    , is_(std::move(is))
    , expectedContentLength_(length)
    , nameUtf_(name)
    , attributeName_(std::move(attrName))
{}

// ---- Static entry point ----

const Attribute* AttributeReader::readAttribute(ClassFileReader& reader, ClassBuilder& builder,
                                                IndexableByteStream& is, AttributeContext context)
{
    std::string attributeName;
    int         expectedContentLength = -1;
    try {
        // Read header
        int            nameIndex = is.readUnsignedShort();
        const CpEntry* nameEntry = builder.getPool().get(nameIndex);
        const CpUtf8*  nameUtf   = (nameEntry != nullptr && nameEntry->tag() == CpTag::UTF8)
                                       ? static_cast<const CpUtf8*>(nameEntry)
                                       : Placeholders::utf8();
        expectedContentLength    = static_cast<int>(is.readInt());
        // 敌意 attribute content length（负 int）：sub-stream 会得到反转 slice
        // （getBuffer 迭代器倒置 UB），父流 moveToAbsolute 得到负绝对位置
        // （负下标读 UB）。统一在此拒绝。
        if (expectedContentLength < 0) {
            throw std::out_of_range("Attribute content length is negative: " +
                                    std::to_string(expectedContentLength));
        }

        // Create local sub-stream, move parent stream past content
        int                 contentStart = is.getAbsoluteIndex();
        IndexableByteStream sub(is, expectedContentLength);
        // contentStart + length 以 i64 计算再钳到 buffer 上界，避免 int 溢出；
        // 超上界时钳到 size_ 与越过 size_ 等价：下一次读取都会在 require() 抛出。
        long long end   = static_cast<long long>(contentStart) + expectedContentLength;
        long long limit = static_cast<long long>(is.getUnderlyingBuffer().size());
        is.moveToAbsolute(static_cast<int>(end > limit ? limit : end));

        // Build reader with sub-stream
        std::string     attrNameStr = nameUtf->text();
        AttributeReader ar(
            reader, builder, std::move(sub), expectedContentLength, nameUtf, attrNameStr);
        attributeName = ar.getAttributeName();

        const Attribute* attribute = ar.read(context);
        if (attribute == nullptr) return nullptr;

        // Post-read position correction for -noverify cases
        int  readerAbsolutePos    = ar.getAbsoluteReadPosition();
        bool hasCorrectedPosition = false;
        if (context.isMethod()) {
            if (attributeName == AttributeConstants::CODE) {
                is.moveToAbsolute(readerAbsolutePos);
                hasCorrectedPosition = true;
            }
            if (attributeName == AttributeConstants::EXCEPTIONS) {
                is.moveToAbsolute(readerAbsolutePos);
                hasCorrectedPosition = true;
            }
        }
        else if (context.isAttribute()) {
            if (attributeName == AttributeConstants::LOCAL_VARIABLE_TABLE) {
                is.moveToAbsolute(readerAbsolutePos);
                hasCorrectedPosition = true;
            }
        }

        if (!hasCorrectedPosition) {
            int readerRelativePos = ar.getRelativeReadPosition();
            if (readerRelativePos != expectedContentLength) {
                return nullptr;
            }
        }

        // Drop if references placeholder constant pool entries
        for (const CpEntry* cpAccess : attribute->cpAccesses()) {
            if (Placeholders::isOrContainsPlaceholder(cpAccess)) {
                return nullptr;
            }
        }

        return attribute;
    }
    catch (const std::exception&) {
        if (reader.doDropEofAttributes()) {
            return nullptr;
        }
        throw;
    }
}

// ---- Internal read dispatch ----

std::vector<const Attribute*> AttributeReader::readAttributes(ClassFileReader&     reader,
                                                              ClassBuilder&        builder,
                                                              IndexableByteStream& is, int count,
                                                              AttributeContext context)
{
    std::vector<const Attribute*> attributes;
    attributes.reserve(static_cast<std::size_t>(count > 0 ? count : 0));
    std::unordered_map<std::string, AttrKind> singletonTypes;
    std::unordered_set<std::string>           conflictingSingletons;
    for (int i = 0; i < count; i++) {
        const Attribute* attribute = readAttribute(reader, builder, is, context);
        if (attribute == nullptr) continue;

        // Some attributes are only allowed to exist once per holder table.
        // If we find multiple of the same type, we will keep the first one, and
        // discard the rest. C++ 无 RTTI：以 AttrKind 替代 Java 的 Class<?> 比较。
        const std::string& attributeName = attribute->name().text();
        if (!isSingletonAttribute(attributeName)) {
            attributes.push_back(attribute);
            continue;
        }

        // Skip any attributes that have already been marked as conflicting.
        if (conflictingSingletons.count(attributeName) > 0) continue;

        // If we have already seen a singleton attribute of this name, check that the
        // type matches.
        AttrKind kind          = attribute->kind();
        auto     firstType     = singletonTypes.find(attributeName);
        if (firstType == singletonTypes.end()) {
            singletonTypes.emplace(attributeName, kind);
            attributes.push_back(attribute);
        }
        else if (firstType->second == kind) {
            // Discarding duplicate singleton attribute.
        }
        else {
            // Discarding incompatible singleton attributes: drop every seen instance
            // of the name and mark the name as conflicting.
            attributes.erase(std::remove_if(attributes.begin(),
                                            attributes.end(),
                                            [&attributeName](const Attribute* existing) {
                                                return existing->name().text() == attributeName;
                                            }),
                             attributes.end());
            singletonTypes.erase(attributeName);
            conflictingSingletons.insert(attributeName);
        }
    }
    return attributes;
}

bool AttributeReader::isSingletonAttribute(std::string_view attributeName)
{
    // JVMS 4.7："must only have one of..." 约束的属性（上游 baad3a72 名单）。
    using C = AttributeConstants;
    return attributeName == C::CODE || attributeName == C::CONSTANT_VALUE ||
           attributeName == C::EXCEPTIONS || attributeName == C::INNER_CLASSES ||
           attributeName == C::ENCLOSING_METHOD || attributeName == C::SIGNATURE ||
           attributeName == C::SOURCE_FILE || attributeName == C::SOURCE_DEBUG_EXTENSION ||
           attributeName == C::RUNTIME_VISIBLE_ANNOTATIONS ||
           attributeName == C::RUNTIME_INVISIBLE_ANNOTATIONS ||
           attributeName == C::RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS ||
           attributeName == C::RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS ||
           attributeName == C::RUNTIME_VISIBLE_TYPE_ANNOTATIONS ||
           attributeName == C::RUNTIME_INVISIBLE_TYPE_ANNOTATIONS ||
           attributeName == C::ANNOTATION_DEFAULT || attributeName == C::BOOTSTRAP_METHODS ||
           attributeName == C::METHOD_PARAMETERS || attributeName == C::MODULE ||
           attributeName == C::MODULE_PACKAGES || attributeName == C::MODULE_MAIN_CLASS ||
           attributeName == C::NEST_HOST || attributeName == C::NEST_MEMBERS ||
           attributeName == C::RECORD || attributeName == C::PERMITTED_SUBCLASSES ||
           attributeName == C::STACK_MAP_TABLE;
}

bool AttributeReader::isModuleAttribute(std::string_view attributeName)
{
    // We only expect to see these in a module-info.class.
    // If we see them elsewhere, it's junk.（上游 fd36c0a7 / aadd6fcf）
    using C = AttributeConstants;
    return attributeName == C::MODULE || attributeName == C::MODULE_HASHES ||
           attributeName == C::MODULE_MAIN_CLASS || attributeName == C::MODULE_PACKAGES ||
           attributeName == C::MODULE_RESOLUTION || attributeName == C::MODULE_TARGET;
}

const Attribute* AttributeReader::read(AttributeContext context)
{
    // Check for forward-versioned attributes
    if (reader_.doDropForwardVersioned()) {
        int introducedAt = AttributeVersions::getIntroducedVersion(attributeName_);
        if (introducedAt > builder_.getVersionMajor()) {
            return nullptr;
        }
    }

    // Check for bad-context attributes
    if (reader_.doDropBadContextAttributes()) {
        auto allowedContexts = AttributeContexts::getAllowedContexts(attributeName_);
        bool found           = false;
        for (auto allowed : allowedContexts) {
            if (allowed == context.type()) {
                found = true;
                break;
            }
        }
        if (!found) {
            return nullptr;
        }
        // Check module attributes in non-module classes（上游 fd36c0a7：按名集中
        // 校验，取代名称前缀特判）
        if (!builder_.isModule() && isModuleAttribute(attributeName_)) {
            return nullptr;
        }
    }

    // Code cannot exist on a method in an annotation type
    if (builder_.isAnnotation() && attributeName_ == AttributeConstants::CODE) {
        return nullptr;
    }

    // Dispatch by attribute name
    if (attributeName_ == AttributeConstants::CODE) {
        return readCode(context);
    }
    else if (attributeName_ == AttributeConstants::CONSTANT_VALUE) {
        if (!(context.memberAccess() & 0x0008)) return nullptr;
        return readConstantValue();
    }
    else if (attributeName_ == AttributeConstants::DEPRECATED) {
        return readDeprecated();
    }
    else if (attributeName_ == AttributeConstants::ENCLOSING_METHOD) {
        return readEnclosingMethod();
    }
    else if (attributeName_ == AttributeConstants::EXCEPTIONS) {
        return readExceptions();
    }
    else if (attributeName_ == AttributeConstants::INNER_CLASSES) {
        return readInnerClasses();
    }
    else if (attributeName_ == AttributeConstants::NEST_HOST) {
        return readNestHost();
    }
    else if (attributeName_ == AttributeConstants::NEST_MEMBERS) {
        return readNestMembers();
    }
    else if (attributeName_ == AttributeConstants::SOURCE_DEBUG_EXTENSION) {
        return readSourceDebugExtension();
    }
    else if (attributeName_ == AttributeConstants::RUNTIME_INVISIBLE_ANNOTATIONS) {
        return readAnnotations(context, false);
    }
    else if (attributeName_ == AttributeConstants::RUNTIME_VISIBLE_ANNOTATIONS) {
        return readAnnotations(context, true);
    }
    else if (attributeName_ == AttributeConstants::RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS) {
        return readParameterAnnotations(context, false);
    }
    else if (attributeName_ == AttributeConstants::RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS) {
        return readParameterAnnotations(context, true);
    }
    else if (attributeName_ == AttributeConstants::RUNTIME_INVISIBLE_TYPE_ANNOTATIONS) {
        return readTypeAnnotations(context, false);
    }
    else if (attributeName_ == AttributeConstants::RUNTIME_VISIBLE_TYPE_ANNOTATIONS) {
        return readTypeAnnotations(context, true);
    }
    else if (attributeName_ == AttributeConstants::ANNOTATION_DEFAULT) {
        return readAnnotationDefault(context);
    }
    else if (attributeName_ == AttributeConstants::SYNTHETIC) {
        return readSynthetic();
    }
    else if (attributeName_ == AttributeConstants::BOOTSTRAP_METHODS) {
        return readBootstrapMethods();
    }
    else if (attributeName_ == AttributeConstants::SIGNATURE) {
        return readSignature();
    }
    else if (attributeName_ == AttributeConstants::SOURCE_FILE) {
        return readSourceFile();
    }
    else if (attributeName_ == AttributeConstants::METHOD_PARAMETERS) {
        return readMethodParameters();
    }
    else if (attributeName_ == AttributeConstants::MODULE) {
        return readModule();
    }
    else if (attributeName_ == AttributeConstants::MODULE_MAIN_CLASS) {
        return readModuleMainClass();
    }
    else if (attributeName_ == AttributeConstants::MODULE_PACKAGES) {
        return readModulePackages();
    }
    else if (attributeName_ == AttributeConstants::MODULE_TARGET) {
        return readModuleTarget();
    }
    else if (attributeName_ == AttributeConstants::MODULE_HASHES) {
        return readModuleHashes();
    }
    else if (attributeName_ == AttributeConstants::MODULE_RESOLUTION) {
        return readModuleResolution();
    }
    else if (attributeName_ == AttributeConstants::STACK_MAP_TABLE) {
        return readStackMapTable();
    }
    else if (attributeName_ == AttributeConstants::LINE_NUMBER_TABLE) {
        return readLineNumbers();
    }
    else if (attributeName_ == AttributeConstants::LOCAL_VARIABLE_TABLE) {
        return readLocalVariables();
    }
    else if (attributeName_ == AttributeConstants::LOCAL_VARIABLE_TYPE_TABLE) {
        return readLocalVariableTypes();
    }
    else if (attributeName_ == AttributeConstants::PERMITTED_SUBCLASSES) {
        return readPermittedClasses();
    }
    else if (attributeName_ == AttributeConstants::RECORD) {
        return readRecord();
    }
    else if (attributeName_ == AttributeConstants::COMPILATION_ID) {
        return readCompileId();
    }
    else if (attributeName_ == AttributeConstants::SOURCE_ID) {
        return readSourceId();
    }
    else if (attributeName_ == AttributeConstants::CHARACTER_RANGE_TABLE) {
        return readCharacterRangeTable();
    }

    // Unknown attribute - skip remaining bytes and return raw data
    if (expectedContentLength_ < 2) {
        is_.skipBytes(expectedContentLength_);
        return nullptr;
    }
    std::vector<ca::u8> raw = is_.getBuffer();
    is_.skipBytes(expectedContentLength_);
    return builder_.createAttribute<DefaultAttribute>(nameUtf_, std::move(raw));
}

// ---- Attribute-specific readers ----

const Attribute* AttributeReader::readCode(AttributeContext context)
{
    int maxStack   = -1;
    int maxLocals  = -1;
    int codeLength = -1;

    if (builder_.isOakVersion()) {
        maxStack   = is_.readUnsignedByte();
        maxLocals  = is_.readUnsignedByte();
        codeLength = is_.readUnsignedShort();
    }
    else {
        maxStack   = is_.readUnsignedShort();
        maxLocals  = is_.readUnsignedShort();
        codeLength = static_cast<int>(is_.readInt());
    }

    // JVMS 4.7.3 要求 code 数组非空且短于 65536 字节；codeLength 来自 readInt()
    // 可为负，旧实现只查 > 65536，负值/0 均漏过（上游 3e9af780）。
    if (reader_.doCheckCodeLength() && (codeLength <= 0 || codeLength >= 65536)) {
        throw std::runtime_error("Method code_length out of range: " + std::to_string(codeLength));
    }

    InstructionReader insnReader(reader_.getFallbackInstructionReader(builder_));
    auto              instructions = insnReader.read(is_, cp_, codeLength);

    int                                             numExceptions = is_.readUnsignedShort();
    std::vector<CodeAttribute::ExceptionTableEntry> exceptions;
    exceptions.reserve(static_cast<std::size_t>(numExceptions));
    for (int i = 0; i < numExceptions; i++) {
        auto entry = readCodeException(codeLength);
        if (entry.has_value()) {
            exceptions.push_back(std::move(*entry));
        }
    }

    int           numAttributes = is_.readUnsignedShort();
    AttributeContext attrContext(AttributeHolderType::ATTRIBUTE, context.memberAccess());
    std::vector<const Attribute*> attributes =
        readAttributes(reader_, builder_, is_, numAttributes, attrContext);

    // Instructions 转入解析 arena（唯一所有权），CodeAttribute 持非拥有视图。
    std::vector<const Instruction*> insnPtrs;
    insnPtrs.reserve(instructions.size());
    for (auto& insn : instructions) {
        insnPtrs.push_back(builder_.adoptInstruction(std::move(insn)));
    }

    return builder_.createAttribute<CodeAttribute>(nameUtf_,
                             maxStack,
                             maxLocals,
                             std::move(insnPtrs),
                             std::move(exceptions),
                             std::move(attributes));
}

const Attribute* AttributeReader::readConstantValue()
{
    const CpEntry* value = cp_.get(is_.readUnsignedShort());
    return builder_.createAttribute<ConstantValueAttribute>(nameUtf_, value);
}

const Attribute* AttributeReader::readDeprecated()
{
    return builder_.createAttribute<DeprecatedAttribute>(nameUtf_);
}

const Attribute* AttributeReader::readEnclosingMethod()
{
    const CpEntry*    enclosingClass  = cp_.get(is_.readUnsignedShort());
    const CpNameType* enclosingMethod = asNameTypeOrNull(cp_, is_.readUnsignedShort());
    if (enclosingClass != nullptr && enclosingClass->tag() == CpTag::CLASS) {
        return builder_.createAttribute<EnclosingMethodAttribute>(
            nameUtf_, static_cast<const CpClass*>(enclosingClass), enclosingMethod);
    }
    return nullptr;
}

const Attribute* AttributeReader::readExceptions()
{
    int                         numberOfExceptionIndices = is_.readUnsignedShort();
    std::vector<const CpClass*> exceptions;
    exceptions.reserve(static_cast<std::size_t>(numberOfExceptionIndices));
    for (int i = 0; i < numberOfExceptionIndices; i++) {
        const CpEntry* exceptionEntry = cp_.get(is_.readUnsignedShort());
        if (exceptionEntry != nullptr && exceptionEntry->tag() == CpTag::CLASS) {
            auto* exceptionClass = static_cast<const CpClass*>(exceptionEntry);
            if (std::find(exceptions.begin(), exceptions.end(), exceptionClass) ==
                exceptions.end()) {
                exceptions.push_back(exceptionClass);
            }
        }
    }
    return builder_.createAttribute<ExceptionsAttribute>(nameUtf_, std::move(exceptions));
}

const Attribute* AttributeReader::readInnerClasses()
{
    int                                            numberOfInnerClasses = is_.readUnsignedShort();
    std::vector<InnerClassesAttribute::InnerClass> innerClasses;
    innerClasses.reserve(static_cast<std::size_t>(numberOfInnerClasses));
    for (int i = 0; i < numberOfInnerClasses; i++) {
        const CpClass* innerClass            = asClassOrNull(cp_, is_.readUnsignedShort());
        const CpClass* outerClass            = asClassOrNull(cp_, is_.readUnsignedShort());
        const CpUtf8*  innerName             = asUtf8OrNull(cp_, is_.readUnsignedShort());
        int            innerClassAccessFlags = is_.readUnsignedShort();
        if (innerClass == nullptr) continue;
        innerClasses.emplace_back(innerClass, outerClass, innerName, innerClassAccessFlags);
    }
    return builder_.createAttribute<InnerClassesAttribute>(nameUtf_, std::move(innerClasses));
}

const Attribute* AttributeReader::readNestHost()
{
    if (expectedContentLength_ != 2) {
        return nullptr;
    }
    const CpClass* nestHost = asClassOrNull(cp_, is_.readUnsignedShort());
    if (nestHost == nullptr) return nullptr;
    return builder_.createAttribute<NestHostAttribute>(nameUtf_, nestHost);
}

const Attribute* AttributeReader::readNestMembers()
{
    int                         count = is_.readUnsignedShort();
    std::vector<const CpClass*> memberClasses;
    memberClasses.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpEntry* memberEntry = cp_.get(is_.readUnsignedShort());
        if (memberEntry != nullptr && memberEntry->tag() == CpTag::CLASS) {
            auto* memberClass = static_cast<const CpClass*>(memberEntry);
            if (std::find(memberClasses.begin(), memberClasses.end(), memberClass) ==
                memberClasses.end()) {
                memberClasses.push_back(memberClass);
            }
        }
    }
    return builder_.createAttribute<NestMembersAttribute>(nameUtf_, std::move(memberClasses));
}

const Attribute* AttributeReader::readSourceDebugExtension()
{
    std::vector<ca::u8> debugExtension;
    is_.readFully(debugExtension, expectedContentLength_);
    return builder_.createAttribute<SourceDebugExtensionAttribute>(nameUtf_, std::move(debugExtension));
}

const Attribute* AttributeReader::readAnnotations(AttributeContext context, bool visible)
{
    AnnotationReader ar(reader_, builder_, is_, expectedContentLength_, nameUtf_, context, visible);
    return ar.readAnnotations();
}

const Attribute* AttributeReader::readParameterAnnotations(AttributeContext context, bool visible)
{
    AnnotationReader ar(reader_, builder_, is_, expectedContentLength_, nameUtf_, context, visible);
    return ar.readParameterAnnotations();
}

const Attribute* AttributeReader::readTypeAnnotations(AttributeContext context, bool visible)
{
    AnnotationReader ar(reader_, builder_, is_, expectedContentLength_, nameUtf_, context, visible);
    return ar.readTypeAnnotations();
}

const Attribute* AttributeReader::readAnnotationDefault(AttributeContext context)
{
    AnnotationReader ar(reader_, builder_, is_, expectedContentLength_, nameUtf_, context, true);
    return ar.readAnnotationDefault();
}

const Attribute* AttributeReader::readSynthetic()
{
    return builder_.createAttribute<SyntheticAttribute>(nameUtf_);
}

const Attribute* AttributeReader::readBootstrapMethods()
{
    int                                                     bsmCount = is_.readUnsignedShort();
    std::vector<BootstrapMethodsAttribute::BootstrapMethod> bootstrapMethods;
    bootstrapMethods.reserve(static_cast<std::size_t>(bsmCount));

    // Track which original entries survive so dynamic bsm indices can be remapped below
    // （上游 7cc58e40：跳过畸形条目而不是丢弃整个属性，让 transformer 的逐条
    // 修复逻辑仍能作用于幸存条目）。
    std::vector<bool> retained(static_cast<std::size_t>(bsmCount), false);
    for (int i = 0; i < bsmCount; i++) {
        const CpMethodHandle* methodRef = nullptr;
        {
            int            mhIndex = is_.readUnsignedShort();
            const CpEntry* e       = cp_.get(mhIndex);
            if (e != nullptr && e->tag() == CpTag::METHOD_HANDLE) {
                methodRef = static_cast<const CpMethodHandle*>(e);
            }
        }
        int                         argCount = is_.readUnsignedShort();
        std::vector<const CpEntry*> args;
        args.reserve(static_cast<std::size_t>(argCount));
        bool hasNullArg = false;
        for (int j = 0; j < argCount; j++) {
            int            argIndex = is_.readUnsignedShort();
            const CpEntry* arg      = cp_.get(argIndex);
            if (arg == nullptr) {
                hasNullArg = true;
            }
            args.push_back(arg);
        }
        if (methodRef == nullptr || hasNullArg) {
            continue;
        }
        retained[static_cast<std::size_t>(i)] = true;
        bootstrapMethods.emplace_back(methodRef, std::move(args));
    }

    // If the table declared entries but none could be retained, drop it entirely.
    if (bootstrapMethods.empty()) {
        return nullptr;
    }

    // Rewrite dynamic bsm indices to account for the entries we skipped.
    transform::IllegalStrippingTransformer::remapBootstrapMethodIndices(retained, cp_);

    return builder_.createAttribute<BootstrapMethodsAttribute>(nameUtf_, std::move(bootstrapMethods));
}

const Attribute* AttributeReader::readSignature()
{
    const CpUtf8* signature = asUtf8OrNull(cp_, is_.readUnsignedShort());
    if (signature == nullptr) return nullptr;
    return builder_.createAttribute<SignatureAttribute>(nameUtf_, signature);
}

const Attribute* AttributeReader::readSourceFile()
{
    const CpEntry* sourceEntry = cp_.get(is_.readUnsignedShort());
    if (sourceEntry != nullptr && sourceEntry->tag() == CpTag::UTF8) {
        return builder_.createAttribute<SourceFileAttribute>(nameUtf_, static_cast<const CpUtf8*>(sourceEntry));
    }
    return nullptr;
}

const Attribute* AttributeReader::readMethodParameters()
{
    int                                               count = is_.readUnsignedByte();
    std::vector<MethodParametersAttribute::Parameter> entries;
    entries.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpUtf8* name        = asUtf8OrNull(cp_, is_.readUnsignedShort());
        int           accessFlags = is_.readUnsignedShort();
        entries.emplace_back(accessFlags, name);
    }
    return builder_.createAttribute<MethodParametersAttribute>(nameUtf_, std::move(entries));
}

const Attribute* AttributeReader::readModule()
{
    const CpModule* module = asModuleOrNull(cp_, is_.readUnsignedShort());
    if (module == nullptr) return nullptr;
    int           flags   = static_cast<int>(is_.readUnsignedShort());
    const CpUtf8* version = asUtf8OrNull(cp_, is_.readUnsignedShort());

    int                                    count = is_.readUnsignedShort();
    std::vector<ModuleAttribute::Requires> requiresList;
    requiresList.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpModule* reqModule = asModuleOrNull(cp_, is_.readUnsignedShort());
        if (reqModule != nullptr) {
            int           reqFlags   = static_cast<int>(is_.readUnsignedShort());
            const CpUtf8* reqVersion = asUtf8OrNull(cp_, is_.readUnsignedShort());
            requiresList.emplace_back(reqModule, reqFlags, reqVersion);
        }
    }

    count = is_.readUnsignedShort();
    std::vector<ModuleAttribute::Exports> exportsList;
    exportsList.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpPackage* expPackage = asPackageOrNull(cp_, is_.readUnsignedShort());
        if (expPackage != nullptr) {
            int                          expFlags = static_cast<int>(is_.readUnsignedShort());
            int                          expCount = is_.readUnsignedShort();
            std::vector<const CpModule*> expModules;
            expModules.reserve(static_cast<std::size_t>(expCount));
            for (int j = 0; j < expCount; j++) {
                const CpModule* expModule = asModuleOrNull(cp_, is_.readUnsignedShort());
                if (expModule != nullptr) expModules.push_back(expModule);
            }
            exportsList.emplace_back(expPackage, expFlags, std::move(expModules));
        }
    }

    count = is_.readUnsignedShort();
    std::vector<ModuleAttribute::Opens> opensList;
    opensList.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpPackage* openPackage = asPackageOrNull(cp_, is_.readUnsignedShort());
        if (openPackage != nullptr) {
            int                          openFlags = static_cast<int>(is_.readUnsignedShort());
            int                          openCount = is_.readUnsignedShort();
            std::vector<const CpModule*> openModules;
            openModules.reserve(static_cast<std::size_t>(openCount));
            for (int j = 0; j < openCount; j++) {
                const CpModule* openModule = asModuleOrNull(cp_, is_.readUnsignedShort());
                if (openModule != nullptr) openModules.push_back(openModule);
            }
            opensList.emplace_back(openPackage, openFlags, std::move(openModules));
        }
    }

    count = is_.readUnsignedShort();
    std::vector<const CpClass*> uses;
    uses.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpClass* useClass = asClassOrNull(cp_, is_.readUnsignedShort());
        if (useClass != nullptr) uses.push_back(useClass);
    }

    count = is_.readUnsignedShort();
    std::vector<ModuleAttribute::Provides> providesList;
    providesList.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpClass* service = asClassOrNull(cp_, is_.readUnsignedShort());
        if (service != nullptr) {
            int                         prvCount = is_.readUnsignedShort();
            std::vector<const CpClass*> providers;
            providers.reserve(static_cast<std::size_t>(prvCount));
            for (int j = 0; j < prvCount; j++) {
                const CpClass* provider = asClassOrNull(cp_, is_.readUnsignedShort());
                if (provider != nullptr) providers.push_back(provider);
            }
            providesList.emplace_back(service, std::move(providers));
        }
    }

    return builder_.createAttribute<ModuleAttribute>(nameUtf_,
                               module,
                               flags,
                               version,
                               std::move(requiresList),
                               std::move(exportsList),
                               std::move(opensList),
                               std::move(uses),
                               std::move(providesList));
}

const Attribute* AttributeReader::readModuleMainClass()
{
    const CpClass* mainClass = asClassOrNull(cp_, is_.readUnsignedShort());
    if (mainClass == nullptr) return nullptr;
    return builder_.createAttribute<ModuleMainClassAttribute>(nameUtf_, mainClass);
}

const Attribute* AttributeReader::readModulePackages()
{
    int                           count = is_.readUnsignedShort();
    std::vector<const CpPackage*> packages;
    packages.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpPackage* pkg = asPackageOrNull(cp_, is_.readUnsignedShort());
        if (pkg != nullptr) packages.push_back(pkg);
    }
    return builder_.createAttribute<ModulePackagesAttribute>(nameUtf_, std::move(packages));
}

const Attribute* AttributeReader::readModuleTarget()
{
    const CpUtf8* platformName = asUtf8OrNull(cp_, is_.readUnsignedShort());
    if (platformName == nullptr) return nullptr;
    return builder_.createAttribute<ModuleTargetAttribute>(nameUtf_, platformName);
}

const Attribute* AttributeReader::readModuleHashes()
{
    const CpUtf8* algorithm = asUtf8OrNull(cp_, is_.readUnsignedShort());
    if (algorithm == nullptr) return nullptr;
    int                                                    moduleCount = is_.readUnsignedShort();
    std::unordered_map<const CpUtf8*, std::vector<ca::u8>> moduleHashes;
    for (int i = 0; i < moduleCount; i++) {
        const CpUtf8*       moduleName       = asUtf8OrNull(cp_, is_.readUnsignedShort());
        int                 moduleHashLength = is_.readUnsignedShort();
        std::vector<ca::u8> moduleHash;
        is_.readFully(moduleHash, moduleHashLength);
        if (moduleName != nullptr) {
            moduleHashes.emplace(moduleName, std::move(moduleHash));
        }
    }
    return builder_.createAttribute<ModuleHashesAttribute>(nameUtf_, algorithm, std::move(moduleHashes));
}

const Attribute* AttributeReader::readModuleResolution()
{
    int flags = is_.readUnsignedShort();
    return builder_.createAttribute<ModuleResolutionAttribute>(nameUtf_, flags);
}

const Attribute* AttributeReader::readStackMapTable()
{
    int numEntries = is_.readUnsignedShort();
    std::vector<std::unique_ptr<StackMapTableAttribute::StackMapFrame>> frames;
    frames.reserve(static_cast<std::size_t>(numEntries));
    for (int i = 0; i < numEntries; i++) {
        int frameType = is_.readUnsignedByte();
        if (frameType <= StackMapTableConstants::SAME_FRAME_MAX) {
            frames.push_back(std::make_unique<StackMapTableAttribute::SameFrame>(frameType));
        }
        else if (frameType <= StackMapTableConstants::SAME_LOCALS_ONE_STACK_ITEM_MAX) {
            auto* stack = readVerificationTypeInfo();
            frames.push_back(std::make_unique<StackMapTableAttribute::SameLocalsOneStackItem>(
                frameType - 64, std::unique_ptr<StackMapTableAttribute::TypeInfo>(stack)));
        }
        else if (frameType < StackMapTableConstants::SAME_LOCALS_ONE_STACK_ITEM_EXTENDED_MIN) {
            throw std::invalid_argument("Unknown stackframe tag " + std::to_string(frameType));
        }
        else if (frameType <= StackMapTableConstants::SAME_LOCALS_ONE_STACK_ITEM_EXTENDED_MAX) {
            int   offsetDelta = is_.readUnsignedShort();
            auto* stack       = readVerificationTypeInfo();
            frames.push_back(
                std::make_unique<StackMapTableAttribute::SameLocalsOneStackItemExtended>(
                    offsetDelta, std::unique_ptr<StackMapTableAttribute::TypeInfo>(stack)));
        }
        else if (frameType <= StackMapTableConstants::CHOP_FRAME_MAX) {
            int k           = 251 - frameType;
            int offsetDelta = is_.readUnsignedShort();
            frames.push_back(std::make_unique<StackMapTableAttribute::ChopFrame>(offsetDelta, k));
        }
        else if (frameType < 252) {
            int offsetDelta = is_.readUnsignedShort();
            frames.push_back(
                std::make_unique<StackMapTableAttribute::SameFrameExtended>(offsetDelta));
        }
        else if (frameType <= StackMapTableConstants::APPEND_FRAME_MAX) {
            int offsetDelta = is_.readUnsignedShort();
            int numLocals   = frameType - 251;
            std::vector<std::unique_ptr<StackMapTableAttribute::TypeInfo>> locals;
            locals.reserve(static_cast<std::size_t>(numLocals));
            for (int j = 0; j < numLocals; j++) {
                locals.push_back(
                    std::unique_ptr<StackMapTableAttribute::TypeInfo>(readVerificationTypeInfo()));
            }
            frames.push_back(std::make_unique<StackMapTableAttribute::AppendFrame>(
                offsetDelta, std::move(locals)));
        }
        else if (frameType <= StackMapTableConstants::FULL_FRAME_MAX) {
            int offsetDelta = is_.readUnsignedShort();
            int numLocals   = is_.readUnsignedShort();
            std::vector<std::unique_ptr<StackMapTableAttribute::TypeInfo>> locals;
            locals.reserve(static_cast<std::size_t>(numLocals));
            for (int j = 0; j < numLocals; j++) {
                locals.push_back(
                    std::unique_ptr<StackMapTableAttribute::TypeInfo>(readVerificationTypeInfo()));
            }
            int numStackItems = is_.readUnsignedShort();
            std::vector<std::unique_ptr<StackMapTableAttribute::TypeInfo>> stack;
            stack.reserve(static_cast<std::size_t>(numStackItems));
            for (int j = 0; j < numStackItems; j++) {
                stack.push_back(
                    std::unique_ptr<StackMapTableAttribute::TypeInfo>(readVerificationTypeInfo()));
            }
            frames.push_back(std::make_unique<StackMapTableAttribute::FullFrame>(
                offsetDelta, std::move(locals), std::move(stack)));
        }
        else {
            throw std::invalid_argument("Unknown frame type " + std::to_string(frameType));
        }
    }
    return builder_.createAttribute<StackMapTableAttribute>(nameUtf_, std::move(frames));
}

StackMapTableAttribute::TypeInfo* AttributeReader::readVerificationTypeInfo()
{
    int tag = is_.readUnsignedByte();
    switch (tag) {
    case StackMapTableConstants::ITEM_TOP: return new StackMapTableAttribute::TopVariableInfo();
    case StackMapTableConstants::ITEM_INTEGER:
        return new StackMapTableAttribute::IntegerVariableInfo();
    case StackMapTableConstants::ITEM_FLOAT: return new StackMapTableAttribute::FloatVariableInfo();
    case StackMapTableConstants::ITEM_DOUBLE:
        return new StackMapTableAttribute::DoubleVariableInfo();
    case StackMapTableConstants::ITEM_LONG: return new StackMapTableAttribute::LongVariableInfo();
    case StackMapTableConstants::ITEM_NULL: return new StackMapTableAttribute::NullVariableInfo();
    case StackMapTableConstants::ITEM_UNINITIALIZED_THIS:
        return new StackMapTableAttribute::UninitializedThisVariableInfo();
    case StackMapTableConstants::ITEM_OBJECT:
    {
        const CpClass* classEntry = asClassOrNull(cp_, is_.readUnsignedShort());
        return new StackMapTableAttribute::ObjectVariableInfo(classEntry);
    }
    case StackMapTableConstants::ITEM_UNINITIALIZED:
    {
        int offset = is_.readUnsignedShort();
        return new StackMapTableAttribute::UninitializedVariableInfo(offset);
    }
    default: throw std::invalid_argument("Unknown verification type tag " + std::to_string(tag));
    }
}

const Attribute* AttributeReader::readLineNumbers()
{
    int                                              count = is_.readUnsignedShort();
    std::vector<LineNumberTableAttribute::LineEntry> entries;
    entries.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        int startPc = is_.readUnsignedShort();
        int line    = is_.readUnsignedShort();
        entries.emplace_back(startPc, line);
    }
    return builder_.createAttribute<LineNumberTableAttribute>(nameUtf_, std::move(entries));
}

const Attribute* AttributeReader::readLocalVariables()
{
    int                                                count = is_.readUnsignedShort();
    std::vector<LocalVariableTableAttribute::VarEntry> entries;
    entries.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        int           startPc   = is_.readUnsignedShort();
        int           length    = is_.readUnsignedShort();
        int           nameIndex = is_.readUnsignedShort();
        int           descIndex = is_.readUnsignedShort();
        int           index     = is_.readUnsignedShort();
        const CpUtf8* name      = asUtf8OrNull(cp_, nameIndex);
        const CpUtf8* desc      = asUtf8OrNull(cp_, descIndex);
        if (name != nullptr && desc != nullptr) {
            entries.emplace_back(startPc, length, name, desc, index);
        }
    }
    return builder_.createAttribute<LocalVariableTableAttribute>(nameUtf_, std::move(entries));
}

const Attribute* AttributeReader::readLocalVariableTypes()
{
    int                                                        count = is_.readUnsignedShort();
    std::vector<LocalVariableTypeTableAttribute::VarTypeEntry> entries;
    entries.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        int           startPc        = is_.readUnsignedShort();
        int           length         = is_.readUnsignedShort();
        int           nameIndex      = is_.readUnsignedShort();
        int           signatureIndex = is_.readUnsignedShort();
        int           index          = is_.readUnsignedShort();
        const CpUtf8* name           = asUtf8OrNull(cp_, nameIndex);
        const CpUtf8* sig            = asUtf8OrNull(cp_, signatureIndex);
        if (name != nullptr && sig != nullptr) {
            entries.emplace_back(startPc, length, name, sig, index);
        }
    }
    return builder_.createAttribute<LocalVariableTypeTableAttribute>(nameUtf_, std::move(entries));
}

const Attribute* AttributeReader::readPermittedClasses()
{
    int                         count = is_.readUnsignedShort();
    std::vector<const CpClass*> entries;
    entries.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpClass* entry = asClassOrNull(cp_, is_.readUnsignedShort());
        if (entry != nullptr) entries.push_back(entry);
    }
    return builder_.createAttribute<PermittedClassesAttribute>(nameUtf_, std::move(entries));
}

const Attribute* AttributeReader::readRecord()
{
    int                                           count = is_.readUnsignedShort();
    std::vector<RecordAttribute::RecordComponent> components;
    components.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        const CpUtf8*                 name          = asUtf8OrNull(cp_, is_.readUnsignedShort());
        const CpUtf8*                 descriptor    = asUtf8OrNull(cp_, is_.readUnsignedShort());
        int                           numAttributes = is_.readUnsignedShort();
        AttributeContext compContext(AttributeHolderType::RECORD_COMPONENT, 0x0001 | 0x0010);
        std::vector<const Attribute*> attributes =
            readAttributes(reader_, builder_, is_, numAttributes, compContext);
        if (name != nullptr && descriptor != nullptr) {
            components.emplace_back(name, descriptor, std::move(attributes));
        }
    }
    return builder_.createAttribute<RecordAttribute>(nameUtf_, std::move(components));
}

const Attribute* AttributeReader::readCompileId()
{
    const CpUtf8* idEntry = asUtf8OrNull(cp_, is_.readUnsignedShort());
    if (idEntry == nullptr) return nullptr;
    return builder_.createAttribute<CompilationIdAttribute>(nameUtf_, idEntry);
}

const Attribute* AttributeReader::readSourceId()
{
    const CpUtf8* idEntry = asUtf8OrNull(cp_, is_.readUnsignedShort());
    if (idEntry == nullptr) return nullptr;
    return builder_.createAttribute<SourceIdAttribute>(nameUtf_, idEntry);
}

const Attribute* AttributeReader::readCharacterRangeTable()
{
    int                                                           len = is_.readUnsignedShort();
    std::vector<CharacterRangeTableAttribute::CharacterRangeInfo> table;
    table.reserve(static_cast<std::size_t>(len));
    for (int i = 0; i < len; i++) {
        int startPc        = is_.readUnsignedShort();
        int endPc          = is_.readUnsignedShort();
        int charRangeStart = static_cast<int>(is_.readInt());
        int charRangeEnd   = static_cast<int>(is_.readInt());
        int flags          = is_.readUnsignedShort();
        table.emplace_back(startPc, endPc, charRangeStart, charRangeEnd, flags);
    }
    return builder_.createAttribute<CharacterRangeTableAttribute>(nameUtf_, std::move(table));
}

// ---- Sub-readers ----

std::optional<CodeAttribute::ExceptionTableEntry> AttributeReader::readCodeException(int codeLength)
{
    int startPc          = is_.readUnsignedShort();
    int endPc            = is_.readUnsignedShort();
    int handlerPc        = is_.readUnsignedShort();
    int catchTypeCpIndex = is_.readUnsignedShort();

    // 越界检查：handler 必须指向 code 数组内部（严格小于 code_length），
    // end_pc 允许等于 code_length（上游 805f636a）。
    if (startPc >= codeLength || endPc > codeLength || handlerPc >= codeLength) {
        return std::nullopt;
    }
    // 区间必须非空且非逆序；逆序（end_pc < start_pc）条目同样丢弃（上游 805f636a）。
    if (startPc >= endPc) {
        return std::nullopt;
    }

    // catch_type == 0 表示 catch-all（源码层的 Throwable）；其余索引必须能解析到
    // CpClass，否则丢弃整个条目，而不是把 nullptr（catch-all 语义）误入表
    // （上游 31111e04）。
    const CpClass* exceptionType = nullptr;
    if (catchTypeCpIndex != 0) {
        exceptionType = asClassOrNull(cp_, catchTypeCpIndex);
        if (exceptionType == nullptr) return std::nullopt;
    }
    return CodeAttribute::ExceptionTableEntry(startPc, endPc, handlerPc, exceptionType);
}

}   // namespace jclassfile::io
