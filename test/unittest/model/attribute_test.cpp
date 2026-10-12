// 阶段 3 gtest：attribute 体系基础验证
// 覆盖：Attribute 抽象基类 cpAccesses 默认值 = {name}
//      38 个 concrete attribute 的构造 / computeInternalLength / computeCompleteLength
//      AttributeContexts 规则表 / AttributeVersions 版本表 / StackMapTableConstants
//      CodeUtilities::wrap 包装功能

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "libca/core/datatype.hpp"
#include "jclassfile/io/annotation_reader.hpp"
#include "jclassfile/model/annotation/annotation.hpp"
#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/attribute/annotation_default_attribute.hpp"
#include "jclassfile/model/attribute/annotations_attribute.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/attribute/attribute_constants.hpp"
#include "jclassfile/model/attribute/attribute_contexts.hpp"
#include "jclassfile/model/attribute/attribute_versions.hpp"
#include "jclassfile/model/attribute/bootstrap_methods_attribute.hpp"
#include "jclassfile/model/attribute/character_range_table_attribute.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/code_utilities.hpp"
#include "jclassfile/model/attribute/compilation_id_attribute.hpp"
#include "jclassfile/model/attribute/constant_value_attribute.hpp"
#include "jclassfile/model/attribute/default_attribute.hpp"
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
#include "jclassfile/model/constant/cp_method_handle.hpp"
#include "jclassfile/model/constant/cp_module.hpp"
#include "jclassfile/model/constant/cp_package.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/instruction/instruction.hpp"

using namespace jclassfile::model;
using namespace jclassfile::io;

namespace {

// 阶段 3 stub：构造一个最小可用的 Instruction（阶段 5 替换为真实类型）。
class StubInsn : public Instruction
{
public:
    StubInsn(int op, int size)
        : Instruction(op)
        , op_(op)
        , size_(size)
    {}
    int      getOpcode() const noexcept { return op_; }
    InsnKind kind() const noexcept override { return InsnKind::Basic; }
    int      computeSize() const noexcept override { return size_; }

private:
    int op_;
    int size_;
};

// 阶段 3 stub：构造一个最小可用的 Annotation（阶段 6 替换为真实类型）。
class StubAnnotation : public Annotation
{
public:
    explicit StubAnnotation(std::vector<const CpEntry*> refs, int len)
        : refs_(std::move(refs))
        , len_(len)
    {}
    std::vector<const CpEntry*> cpAccesses() const override { return refs_; }
    int                         computeLength() const noexcept override { return len_; }

private:
    std::vector<const CpEntry*> refs_;
    int                         len_;
};

// 阶段 3 stub：构造一个最小可用的 ElementValue（阶段 6 替换为真实类型）。
class StubElementValue : public ElementValue
{
public:
    explicit StubElementValue(std::vector<const CpEntry*> refs, int len)
        : ElementValue('?')
        , refs_(std::move(refs))
        , len_(len)
    {}
    ElementValueKind            kind() const noexcept override { return ElementValueKind::Utf8; }
    std::vector<const CpEntry*> cpAccesses() const override { return refs_; }
    int                         computeLength() const noexcept override { return len_; }

private:
    std::vector<const CpEntry*> refs_;
    int                         len_;
};

class AttributeTest : public ::testing::Test
{
protected:
    void                    SetUp() override { name_ = std::make_unique<CpUtf8>("TestAttr"); }
    std::unique_ptr<CpUtf8> name_;
};

}   // namespace

// ----- Attribute 基类 -----

TEST_F(AttributeTest, BaseCpAccessesIsJustName)
{
    DeprecatedAttribute attr(name_.get());
    auto                set = attr.cpAccesses();
    ASSERT_EQ(set.size(), 1u);
    EXPECT_EQ(set[0], static_cast<const CpEntry*>(name_.get()));
}

TEST_F(AttributeTest, CompleteLengthIsInternalPlusSix)
{
    DeprecatedAttribute attr(name_.get());
    EXPECT_EQ(attr.computeInternalLength(), 0);
    EXPECT_EQ(attr.computeCompleteLength(), 6);
}

// ----- Synthetic / Deprecated -----

TEST_F(AttributeTest, SyntheticHasZeroLength)
{
    SyntheticAttribute attr(name_.get());
    EXPECT_EQ(attr.computeInternalLength(), 0);
    EXPECT_EQ(attr.computeCompleteLength(), 6);
}

TEST_F(AttributeTest, DeprecatedHasZeroLength)
{
    DeprecatedAttribute attr(name_.get());
    EXPECT_EQ(attr.computeInternalLength(), 0);
}

// ----- 持有 CpUtf8 的 attribute -----

TEST_F(AttributeTest, SignatureAttributeLengthIsTwo)
{
    auto               sig = std::make_unique<CpUtf8>("Ljava/util/List<Ljava/lang/String;>;");
    SignatureAttribute attr(name_.get(), sig.get());
    EXPECT_EQ(attr.computeInternalLength(), 2);
    auto set = attr.cpAccesses();
    EXPECT_EQ(set.size(), 2u);   // name + signature
}

TEST_F(AttributeTest, SourceFileAttributeLengthIsTwo)
{
    auto                fname = std::make_unique<CpUtf8>("Foo.java");
    SourceFileAttribute attr(name_.get(), fname.get());
    EXPECT_EQ(attr.computeInternalLength(), 2);
    auto set = attr.cpAccesses();
    EXPECT_EQ(set.size(), 2u);
}

TEST_F(AttributeTest, SourceIdAttributeLengthIsTwo)
{
    auto              sid = std::make_unique<CpUtf8>("abc123");
    SourceIdAttribute attr(name_.get(), sid.get());
    EXPECT_EQ(attr.computeInternalLength(), 2);
}

TEST_F(AttributeTest, CompilationIdAttributeLengthIsTwo)
{
    auto                   cid = std::make_unique<CpUtf8>("jdk-internal");
    CompilationIdAttribute attr(name_.get(), cid.get());
    EXPECT_EQ(attr.computeInternalLength(), 2);
}

TEST_F(AttributeTest, SourceDebugExtensionLengthEqualsData)
{
    SourceDebugExtensionAttribute attr(name_.get(), {0x01, 0x02, 0x03, 0x04});
    EXPECT_EQ(attr.computeInternalLength(), 4);
}

TEST_F(AttributeTest, DefaultAttributeLengthEqualsData)
{
    DefaultAttribute attr(name_.get(), {0xCA, 0xFE, 0xBA, 0xBE});
    EXPECT_EQ(attr.computeInternalLength(), 4);
}

TEST_F(AttributeTest, ConstantValueAttributeLengthIsTwo)
{
    auto                   utf = std::make_unique<CpUtf8>("abc");
    ConstantValueAttribute attr(name_.get(), utf.get());
    EXPECT_EQ(attr.computeInternalLength(), 2);
    auto set = attr.cpAccesses();
    EXPECT_EQ(set.size(), 2u);   // name + value
}

TEST_F(AttributeTest, EnclosingMethodAttributeLengthIsFour)
{
    auto                     cls = std::make_unique<CpUtf8>("Foo");
    CpClass                  host(nullptr);
    EnclosingMethodAttribute attr(name_.get(), &host, nullptr);
    EXPECT_EQ(attr.computeInternalLength(), 4);
    EXPECT_EQ(attr.cpAccesses().size(), 2u);   // name + class (method=null skipped)
}

TEST_F(AttributeTest, ExceptionsAttributeLengthMatchesCount)
{
    CpClass             a(nullptr), b(nullptr);
    ExceptionsAttribute attr(name_.get(), {&a, &b});
    // 2 + 2 * 2
    EXPECT_EQ(attr.computeInternalLength(), 6);
    EXPECT_EQ(attr.cpAccesses().size(), 3u);   // name + 2
}

TEST_F(AttributeTest, InnerClassesAttributeLengthMatchesCount)
{
    CpClass                           c1(nullptr), c2(nullptr);
    CpUtf8                            n1("n1");
    InnerClassesAttribute::InnerClass ic(&c1, &c2, &n1, 0x0001);
    InnerClassesAttribute             attr(name_.get(), {ic});
    // 2 + 1 * 8
    EXPECT_EQ(attr.computeInternalLength(), 10);
    auto set = attr.cpAccesses();
    EXPECT_EQ(set.size(), 4u);   // name + 3 refs (outer/inner/name)
}

TEST_F(AttributeTest, BootstrapMethodsAttributeLength)
{
    CpMethodHandle                             mh(1, nullptr);
    BootstrapMethodsAttribute::BootstrapMethod bsm(&mh, {});
    BootstrapMethodsAttribute                  attr(name_.get(), {bsm});
    // 2 + (4 + 0) = 6
    EXPECT_EQ(attr.computeInternalLength(), 6);
}

TEST_F(AttributeTest, LineNumberTableAttributeLength)
{
    LineNumberTableAttribute::LineEntry e1(0, 1), e2(5, 6);
    LineNumberTableAttribute            attr(name_.get(), {e1, e2});
    // 2 + 2 * 4 = 10
    EXPECT_EQ(attr.computeInternalLength(), 10);
}

TEST_F(AttributeTest, LocalVariableTableAttributeLength)
{
    CpUtf8                                n("x"), d("I");
    LocalVariableTableAttribute::VarEntry ve(0, 5, &n, &d, 1);
    LocalVariableTableAttribute           attr(name_.get(), {ve});
    // 2 + 1 * 10 = 12
    EXPECT_EQ(attr.computeInternalLength(), 12);
    auto set = attr.cpAccesses();
    EXPECT_EQ(set.size(), 3u);   // name + name + desc
}

TEST_F(AttributeTest, LocalVariableTypeTableAttributeLength)
{
    CpUtf8                                        n("x"), s("Ljava/util/List;");
    LocalVariableTypeTableAttribute::VarTypeEntry vte(0, 5, &n, &s, 1);
    LocalVariableTypeTableAttribute               attr(name_.get(), {vte});
    EXPECT_EQ(attr.computeInternalLength(), 12);
}

TEST_F(AttributeTest, MethodParametersAttributeLength)
{
    MethodParametersAttribute::Parameter p(0x0010, nullptr);
    MethodParametersAttribute            attr(name_.get(), {p});
    // 1 + 1 * 4 = 5
    EXPECT_EQ(attr.computeInternalLength(), 5);
    auto set = attr.cpAccesses();
    EXPECT_EQ(set.size(), 1u);   // only name
}

TEST_F(AttributeTest, NestHostAttributeLengthIsTwo)
{
    CpClass           host(nullptr);
    NestHostAttribute attr(name_.get(), &host);
    EXPECT_EQ(attr.computeInternalLength(), 2);
    EXPECT_EQ(attr.cpAccesses().size(), 2u);
}

TEST_F(AttributeTest, NestMembersAttributeLength)
{
    CpClass              a(nullptr), b(nullptr);
    NestMembersAttribute attr(name_.get(), {&a, &b});
    EXPECT_EQ(attr.computeInternalLength(), 6);   // 2 + 2 * 2
    EXPECT_EQ(attr.cpAccesses().size(), 3u);
}

TEST_F(AttributeTest, ModuleHashesAttributeLength)
{
    CpUtf8                                                 algo("SHA-256"), mod("foo");
    std::unordered_map<const CpUtf8*, std::vector<ca::u8>> m;
    std::vector<ca::u8>                                    hash{0x01, 0x02, 0x03};
    m[&mod] = hash;
    ModuleHashesAttribute attr(name_.get(), &algo, std::move(m));
    // 4 (algo + count) + (4 + 3) = 11
    EXPECT_EQ(attr.computeInternalLength(), 11);
    EXPECT_EQ(attr.cpAccesses().size(), 3u);   // name + algo + mod
}

TEST_F(AttributeTest, ModuleMainClassAttributeLengthIsTwo)
{
    CpClass                  main(nullptr);
    ModuleMainClassAttribute attr(name_.get(), &main);
    EXPECT_EQ(attr.computeInternalLength(), 2);
}

TEST_F(AttributeTest, ModulePackagesAttributeLength)
{
    CpPackage               p1(nullptr), p2(nullptr);
    ModulePackagesAttribute attr(name_.get(), {&p1, &p2});
    EXPECT_EQ(attr.computeInternalLength(), 6);
    EXPECT_EQ(attr.cpAccesses().size(), 3u);
}

TEST_F(AttributeTest, ModuleResolutionAttributeLengthIsTwo)
{
    ModuleResolutionAttribute attr(name_.get(), 0x0001);
    EXPECT_EQ(attr.computeInternalLength(), 2);
}

TEST_F(AttributeTest, ModuleTargetAttributeLengthIsTwo)
{
    CpUtf8                platform("linux-x64");
    ModuleTargetAttribute attr(name_.get(), &platform);
    EXPECT_EQ(attr.computeInternalLength(), 2);
}

TEST_F(AttributeTest, PermittedClassesAttributeLength)
{
    CpClass                   a(nullptr), b(nullptr);
    PermittedClassesAttribute attr(name_.get(), {&a, &b});
    EXPECT_EQ(attr.computeInternalLength(), 6);
    EXPECT_EQ(attr.cpAccesses().size(), 3u);
}

TEST_F(AttributeTest, CharacterRangeTableAttributeLength)
{
    CharacterRangeTableAttribute::CharacterRangeInfo info(0, 10, 0, 100, 0x0001);
    CharacterRangeTableAttribute                     attr(name_.get(), {info});
    // 2 (u2 条目数) + 14 * 1；上游 7ddef35d 修复（旧版镜像 bug 误写 2 * (14*1) = 28，
    // 导致 attribute_writer 写出错误 attribute length）
    EXPECT_EQ(attr.computeInternalLength(), 16);
}

// 上游 7ddef35d：多条目时长度必须为 2 + 14*n，与写入字节数一致。
TEST_F(AttributeTest, CharacterRangeTableAttributeLengthMultipleEntries)
{
    using Info = CharacterRangeTableAttribute::CharacterRangeInfo;
    CharacterRangeTableAttribute attr(name_.get(), {Info(0, 5, 0, 100, 0x0001),
                                                    Info(5, 10, 1 << 10, 2 << 10, 0x0002),
                                                    Info(10, 15, 0, 1, 0x0004),
                                                    Info(15, 20, 0, 1, 0x0008)});
    // 2 + 14 * 4 = 58
    EXPECT_EQ(attr.computeInternalLength(), 58);
}

// 上游 3e814f0a：setCharacterRangeEnd(line, column) 旧版误写 characterRangeStart_。
TEST_F(AttributeTest, CharacterRangeTableSetterWritesOwnField)
{
    CharacterRangeTableAttribute::CharacterRangeInfo info(0, 10, 0, 0, 0);

    info.setCharacterRangeStart(3, 7);
    EXPECT_EQ(info.characterRangeStart(), (3 << 10) + 7);
    EXPECT_EQ(info.characterRangeEnd(), 0);

    info.setCharacterRangeEnd(5, 9);
    EXPECT_EQ(info.characterRangeEnd(), (5 << 10) + 9);
    EXPECT_EQ(info.characterRangeStart(), (3 << 10) + 7);   // start 不被 end setter 破坏
}

TEST_F(AttributeTest, ModuleAttributeLength)
{
    CpModule                  mod(nullptr);
    CpClass                   service(nullptr);
    ModuleAttribute::Requires r(&mod, 0x0001, nullptr);
    ModuleAttribute::Exports  e(nullptr, 0x0001, {});
    ModuleAttribute::Opens    o(nullptr, 0x0001, {});
    ModuleAttribute::Provides p(&service, {});
    ModuleAttribute           attr(name_.get(), &mod, 0x0001, nullptr, {r}, {e}, {o}, {}, {p});
    // 6 (name+flags+version) +
    // 2 + 1*6 (requires) +
    // 2 + 6 (exports) +
    // 2 + 6 (opens) +
    // 2 + 0 (uses) +
    // 2 + 4 (provides)
    // = 6 + 8 + 8 + 8 + 2 + 6 = 38
    EXPECT_EQ(attr.computeInternalLength(), 38);
}

TEST_F(AttributeTest, AnnotationsAttributeLength)
{
    std::vector<std::unique_ptr<Annotation>> annos;
    annos.push_back(std::make_unique<StubAnnotation>(std::vector<const CpEntry*>{}, 4));
    annos.push_back(std::make_unique<StubAnnotation>(std::vector<const CpEntry*>{}, 6));
    AnnotationsAttribute attr(name_.get(), std::move(annos), true);
    // 2 + 4 + 6 = 12
    EXPECT_EQ(attr.computeInternalLength(), 12);
}

TEST_F(AttributeTest, ParameterAnnotationsAttributeLength)
{
    std::vector<std::unique_ptr<Annotation>> list1;
    list1.push_back(std::make_unique<StubAnnotation>(std::vector<const CpEntry*>{}, 4));
    std::vector<std::vector<std::unique_ptr<Annotation>>> all;
    all.push_back(std::move(list1));
    all.push_back({});
    ParameterAnnotationsAttribute attr(name_.get(), std::move(all), true);
    // 1 (num_parameters) + (2 + 4) + 2 (empty list) = 9
    EXPECT_EQ(attr.computeInternalLength(), 9);
}

TEST_F(AttributeTest, AnnotationDefaultAttributeLength)
{
    auto ev = std::make_unique<StubElementValue>(std::vector<const CpEntry*>{}, 5);
    AnnotationDefaultAttribute attr(name_.get(), std::move(ev));
    EXPECT_EQ(attr.computeInternalLength(), 5);
    auto set = attr.cpAccesses();
    EXPECT_EQ(set.size(), 1u);   // only name; StubElementValue returns empty
}

TEST_F(AttributeTest, CodeAttributeLength)
{
    std::vector<const Instruction*> insns = {
        new StubInsn(0, 1),   // 1 byte
        new StubInsn(0, 3),   // 3 bytes
    };
    CodeAttribute::ExceptionTableEntry ete(0, 5, 10, nullptr);
    CodeAttribute                      attr(name_.get(), 2, 1, insns, {ete}, {});
    // 4 (max_stack+max_locals) +
    // 4 (code_length) +
    // 4 (insns) +
    // 2 (ex_table_len) +
    // 8 (1 ex) +
    // 2 (attr_count) +
    // 0 (no attrs)
    // = 24
    EXPECT_EQ(attr.computeInternalLength(), 24);
    for (auto* p : insns) delete p;
}

TEST_F(AttributeTest, RecordAttributeLength)
{
    CpUtf8                        cname("x"), cdesc("I");
    std::vector<const Attribute*> compAttrs = {
        new DeprecatedAttribute(name_.get()),
    };
    RecordAttribute::RecordComponent comp(&cname, &cdesc, compAttrs);
    RecordAttribute                  attr(name_.get(), {comp});
    // 2 (count) + (6 + 6 (Deprecated complete)) = 14
    EXPECT_EQ(attr.computeInternalLength(), 14);
    for (auto* p : compAttrs) delete p;
}

TEST_F(AttributeTest, StackMapTableAttributeLength)
{
    auto same    = std::make_unique<StackMapTableAttribute::SameFrame>(0);
    auto sameOne = std::make_unique<StackMapTableAttribute::SameLocalsOneStackItem>(
        0, std::make_unique<StackMapTableAttribute::TopVariableInfo>());
    auto full = std::make_unique<StackMapTableAttribute::FullFrame>(
        0,
        std::vector<std::unique_ptr<StackMapTableAttribute::TypeInfo>>{},
        std::vector<std::unique_ptr<StackMapTableAttribute::TypeInfo>>{});
    std::vector<std::unique_ptr<StackMapTableAttribute::StackMapFrame>> frames;
    frames.push_back(std::move(same));      // 1 byte
    frames.push_back(std::move(sameOne));   // 1 + 1 = 2 bytes
    frames.push_back(std::move(full));      // 1 + 2 + 2 + 2 = 7 bytes
    StackMapTableAttribute attr(name_.get(), std::move(frames));
    // 2 (count) + 1 + 2 + 7 = 12
    EXPECT_EQ(attr.computeInternalLength(), 12);
}

TEST_F(AttributeTest, StackMapTableAppendFrameLength)
{
    auto t1 = std::make_unique<StackMapTableAttribute::TopVariableInfo>();
    auto t2 = std::make_unique<StackMapTableAttribute::IntegerVariableInfo>();
    std::vector<std::unique_ptr<StackMapTableAttribute::TypeInfo>> emptyLocals;
    std::vector<std::unique_ptr<StackMapTableAttribute::TypeInfo>> twoLocals;
    twoLocals.push_back(std::move(t1));
    twoLocals.push_back(std::move(t2));
    auto ap  = std::make_unique<StackMapTableAttribute::AppendFrame>(3, std::move(emptyLocals));
    auto ap2 = std::make_unique<StackMapTableAttribute::AppendFrame>(3, std::move(twoLocals));
    std::vector<std::unique_ptr<StackMapTableAttribute::StackMapFrame>> frames;
    frames.push_back(std::move(ap));
    frames.push_back(std::move(ap2));
    StackMapTableAttribute attr(name_.get(), std::move(frames));
    // 2 + (1+2) + (1+2+1+1) = 2 + 3 + 5 = 10
    EXPECT_EQ(attr.computeInternalLength(), 10);
}

TEST_F(AttributeTest, StackMapTableObjectCpAccess)
{
    CpClass cls(nullptr);
    auto    info = std::make_unique<StackMapTableAttribute::ObjectVariableInfo>(&cls);
    auto    set  = info->cpAccesses();
    ASSERT_EQ(set.size(), 1u);
    EXPECT_EQ(set[0], static_cast<const CpEntry*>(&cls));
}

// ----- AttributeContexts / AttributeVersions 规则表 -----

TEST(AttributeContextsTest, ClassOnlyAttributes)
{
    EXPECT_EQ(AttributeContexts::getAllowedContexts(AttributeConstants::RECORD).size(), 1u);
    EXPECT_EQ(AttributeContexts::getAllowedContexts(AttributeConstants::RECORD)[0],
              AttributeHolderType::CLASS);

    EXPECT_EQ(AttributeContexts::getAllowedContexts(AttributeConstants::MODULE).size(), 1u);
    EXPECT_EQ(AttributeContexts::getAllowedContexts(AttributeConstants::MODULE)[0],
              AttributeHolderType::CLASS);
}

TEST(AttributeContextsTest, FieldOnlyAttributes)
{
    auto v = AttributeContexts::getAllowedContexts(AttributeConstants::CONSTANT_VALUE);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], AttributeHolderType::FIELD);
}

TEST(AttributeContextsTest, MethodOnlyAttributes)
{
    auto v = AttributeContexts::getAllowedContexts(AttributeConstants::CODE);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], AttributeHolderType::METHOD);
}

TEST(AttributeContextsTest, ClassFieldMethodAttributes)
{
    auto v = AttributeContexts::getAllowedContexts(AttributeConstants::DEPRECATED);
    EXPECT_EQ(v.size(), 3u);
    auto v2 = AttributeContexts::getAllowedContexts(AttributeConstants::SYNTHETIC);
    EXPECT_EQ(v2.size(), 3u);
}

TEST(AttributeContextsTest, AttributeOnlyAttributes)
{
    auto v = AttributeContexts::getAllowedContexts(AttributeConstants::STACK_MAP_TABLE);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], AttributeHolderType::ATTRIBUTE);
}

TEST(AttributeContextsTest, AllHolderAttributes)
{
    // 上游 aadd6fcf：普通注解与 Signature 收窄到四成员上下文（不含 ATTRIBUTE 宿主）。
    auto v = AttributeContexts::getAllowedContexts(AttributeConstants::SIGNATURE);
    EXPECT_EQ(v.size(), 4u);
    auto v2 = AttributeContexts::getAllowedContexts(AttributeConstants::RUNTIME_VISIBLE_ANNOTATIONS);
    EXPECT_EQ(v2.size(), 4u);
    // 类型注解仍允许全部宿主。
    auto v3 =
        AttributeContexts::getAllowedContexts(AttributeConstants::RUNTIME_VISIBLE_TYPE_ANNOTATIONS);
    EXPECT_EQ(v3.size(), 5u);
}

TEST(AttributeContextsTest, UnknownAttributeAllowsAll)
{
    auto v = AttributeContexts::getAllowedContexts("CustomAttribute");
    EXPECT_EQ(v.size(), 5u);
}

TEST(AttributeVersionsTest, KnownAttributes)
{
    EXPECT_EQ(AttributeVersions::getIntroducedVersion(AttributeConstants::CODE),
              VersionConstants::JAVA1);
    EXPECT_EQ(AttributeVersions::getIntroducedVersion(AttributeConstants::ENCLOSING_METHOD),
              VersionConstants::JAVA5);
    EXPECT_EQ(AttributeVersions::getIntroducedVersion(AttributeConstants::STACK_MAP_TABLE),
              VersionConstants::JAVA6);
    EXPECT_EQ(AttributeVersions::getIntroducedVersion(AttributeConstants::BOOTSTRAP_METHODS),
              VersionConstants::JAVA7);
    EXPECT_EQ(AttributeVersions::getIntroducedVersion(AttributeConstants::MODULE),
              VersionConstants::JAVA9);
    EXPECT_EQ(AttributeVersions::getIntroducedVersion(AttributeConstants::NEST_HOST),
              VersionConstants::JAVA11);
    EXPECT_EQ(AttributeVersions::getIntroducedVersion(AttributeConstants::RECORD),
              VersionConstants::JAVA14);
    EXPECT_EQ(AttributeVersions::getIntroducedVersion(AttributeConstants::PERMITTED_SUBCLASSES),
              VersionConstants::JAVA15);
}

TEST(AttributeVersionsTest, UnknownReturnsMinusOne)
{
    EXPECT_EQ(AttributeVersions::getIntroducedVersion("Custom"), -1);
}

TEST(StackMapTableConstantsTest, TagValues)
{
    EXPECT_EQ(StackMapTableConstants::ITEM_TOP, 0);
    EXPECT_EQ(StackMapTableConstants::ITEM_INTEGER, 1);
    EXPECT_EQ(StackMapTableConstants::ITEM_OBJECT, 7);
    EXPECT_EQ(StackMapTableConstants::SAME_FRAME_MIN, 0);
    EXPECT_EQ(StackMapTableConstants::SAME_FRAME_MAX, 63);
    EXPECT_EQ(StackMapTableConstants::FULL_FRAME_MIN, 255);
}

TEST(CodeUtilitiesTest, WrapReturnsValidInstance)
{
    std::vector<const Instruction*> insns = {new StubInsn(0, 1)};
    auto                            w     = CodeUtilities::wrap(insns);
    EXPECT_EQ(w->instructions().size(), 1u);
    delete insns[0];
}

TEST(CodeUtilitiesTest, WrapComputeSizeAccumulates)
{
    std::vector<const Instruction*> insns = {new StubInsn(0, 1), new StubInsn(0, 3)};
    auto                            w     = CodeUtilities::wrap(insns);
    EXPECT_EQ(w->computeSize(), 4);
    delete insns[0];
    delete insns[1];
}

TEST(CodeUtilitiesTest, IndexOfByPointer)
{
    auto                            i1    = new StubInsn(0, 1);
    auto                            i2    = new StubInsn(0, 3);
    std::vector<const Instruction*> insns = {i1, i2};
    auto                            w     = CodeUtilities::wrap(insns);
    EXPECT_EQ(w->indexOf(i1), 0);
    EXPECT_EQ(w->indexOf(i2), 1);
    EXPECT_EQ(w->indexOf(nullptr), -1);
    delete i1;
    delete i2;
}

// ---- 上游 66d31cf8：primitive 元素值 tag 反查必须的常量池种类 ----
// 上游落在 PrimitiveElementValue.requiredConstantTag；C++ 侧承载于
// AnnotationReader（model 文件不在本次改动范围）。

TEST(PrimitiveElementValueTest, RequiredConstantTagMapping)
{
    using jclassfile::io::AnnotationReader;
    namespace model = jclassfile::model;
    // B/C/I/S/Z → INTEGER。
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('B'),
              static_cast<int>(model::CpTag::INTEGER));
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('C'),
              static_cast<int>(model::CpTag::INTEGER));
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('I'),
              static_cast<int>(model::CpTag::INTEGER));
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('S'),
              static_cast<int>(model::CpTag::INTEGER));
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('Z'),
              static_cast<int>(model::CpTag::INTEGER));
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('F'),
              static_cast<int>(model::CpTag::FLOAT));
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('J'),
              static_cast<int>(model::CpTag::LONG));
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('D'),
              static_cast<int>(model::CpTag::DOUBLE));
    // 非原始类型 element value tag：-1。
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('s'), -1);
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('e'), -1);
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('c'), -1);
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('@'), -1);
    EXPECT_EQ(AnnotationReader::requiredConstantTagForPrimitive('['), -1);
}
