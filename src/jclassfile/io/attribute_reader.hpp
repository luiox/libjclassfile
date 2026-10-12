#pragma once

#include <optional>
#include <string>

#include "attribute_context.hpp"
#include "class_builder.hpp"
#include "indexable_byte_stream.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/default_attribute.hpp"
#include "jclassfile/model/attribute/stack_map_table_attribute.hpp"
#include "jclassfile/model/attribute/version_constants.hpp"
#include "jclassfile/model/constant/cp_module.hpp"
#include "jclassfile/model/constant/cp_package.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/constant/placeholders.hpp"

namespace jclassfile::io {

class ClassFileReader;

class AttributeReader
{
public:
    [[nodiscard]] static const model::Attribute* readAttribute(ClassFileReader&     reader,
                                                               ClassBuilder&        builder,
                                                               IndexableByteStream& is,
                                                               AttributeContext     context);

    /// @brief 读取一个有界属性表并做单例属性归一化（上游 baad3a72）。
    ///
    /// 部分属性（"must only have one of..." 规范条目）在同一 holder 表中只允许
    /// 出现一次：重复的同名同型属性保留首个、丢弃后来者；同名不同型视为互相
    /// 冲突，全部丢弃。返回值保持原始顺序。
    [[nodiscard]] static std::vector<const model::Attribute*> readAttributes(
        ClassFileReader& reader, ClassBuilder& builder, IndexableByteStream& is, int count,
        AttributeContext context);

    /// @brief 属性名是否限定"每个 holder 表至多一个实例"（上游 baad3a72）。
    [[nodiscard]] static bool isSingletonAttribute(std::string_view attributeName);

    /// @brief 属性名是否为已知的 module 属性（上游 fd36c0a7，按名集中校验）。
    ///
    /// 上游落在 AttributeContexts.isModuleAttribute；该文件不在本次改动范围，
    /// 暂以 AttributeReader 静态方法承载，transformer 侧复用。
    [[nodiscard]] static bool isModuleAttribute(std::string_view attributeName);

    [[nodiscard]] int getAbsoluteReadPosition() const noexcept { return is_.getAbsoluteIndex(); }
    [[nodiscard]] int getRelativeReadPosition() const noexcept { return is_.getIndex(); }
    [[nodiscard]] int getExpectedContentLength() const noexcept { return expectedContentLength_; }
    [[nodiscard]] const std::string& getAttributeName() const noexcept { return attributeName_; }

private:
    AttributeReader(ClassFileReader& reader, ClassBuilder& builder, IndexableByteStream&& is,
                    int length, const model::CpUtf8* name, std::string attrName);

    [[nodiscard]] const model::Attribute* read(AttributeContext context);

    // Attribute-specific readers
    [[nodiscard]] const model::Attribute* readCode(AttributeContext context);
    [[nodiscard]] const model::Attribute* readConstantValue();
    [[nodiscard]] const model::Attribute* readDeprecated();
    [[nodiscard]] const model::Attribute* readEnclosingMethod();
    [[nodiscard]] const model::Attribute* readExceptions();
    [[nodiscard]] const model::Attribute* readInnerClasses();
    [[nodiscard]] const model::Attribute* readNestHost();
    [[nodiscard]] const model::Attribute* readNestMembers();
    [[nodiscard]] const model::Attribute* readSourceDebugExtension();
    [[nodiscard]] const model::Attribute* readAnnotations(AttributeContext context, bool visible);
    [[nodiscard]] const model::Attribute* readParameterAnnotations(AttributeContext context,
                                                                   bool             visible);
    [[nodiscard]] const model::Attribute* readTypeAnnotations(AttributeContext context,
                                                              bool             visible);
    [[nodiscard]] const model::Attribute* readAnnotationDefault(AttributeContext context);
    [[nodiscard]] const model::Attribute* readSynthetic();
    [[nodiscard]] const model::Attribute* readBootstrapMethods();
    [[nodiscard]] const model::Attribute* readSignature();
    [[nodiscard]] const model::Attribute* readSourceFile();
    [[nodiscard]] const model::Attribute* readMethodParameters();
    [[nodiscard]] const model::Attribute* readModule();
    [[nodiscard]] const model::Attribute* readModuleMainClass();
    [[nodiscard]] const model::Attribute* readModulePackages();
    [[nodiscard]] const model::Attribute* readModuleTarget();
    [[nodiscard]] const model::Attribute* readModuleHashes();
    [[nodiscard]] const model::Attribute* readModuleResolution();
    [[nodiscard]] const model::Attribute* readStackMapTable();
    [[nodiscard]] const model::Attribute* readLineNumbers();
    [[nodiscard]] const model::Attribute* readLocalVariables();
    [[nodiscard]] const model::Attribute* readLocalVariableTypes();
    [[nodiscard]] const model::Attribute* readPermittedClasses();
    [[nodiscard]] const model::Attribute* readRecord();
    [[nodiscard]] const model::Attribute* readCompileId();
    [[nodiscard]] const model::Attribute* readSourceId();
    [[nodiscard]] const model::Attribute* readCharacterRangeTable();

    // Sub-readers
    [[nodiscard]] model::StackMapTableAttribute::TypeInfo* readVerificationTypeInfo();
    /// 读取一条异常表项；非法范围 / start == end 时返回 nullopt。值语义，无堆中转。
    [[nodiscard]] std::optional<model::CodeAttribute::ExceptionTableEntry> readCodeException(
        int codeLength);

    // Helpers
    static const model::CpUtf8*     asUtf8OrNull(const model::ConstPool& cp, int index);
    static const model::CpClass*    asClassOrNull(const model::ConstPool& cp, int index);
    static const model::CpNameType* asNameTypeOrNull(const model::ConstPool& cp, int index);
    static const model::CpModule*   asModuleOrNull(const model::ConstPool& cp, int index);
    static const model::CpPackage*  asPackageOrNull(const model::ConstPool& cp, int index);

    ClassFileReader&        reader_;
    ClassBuilder&           builder_;
    const model::ConstPool& cp_;
    IndexableByteStream     is_;
    int                     expectedContentLength_;
    const model::CpUtf8*    nameUtf_;
    std::string             attributeName_;
};

}   // namespace jclassfile::io
