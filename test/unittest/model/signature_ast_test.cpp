// issue #153：Signature AST 读 + 写 round-trip 测试
// 对标 JVMS §4.7.9.1，覆盖 class/method/field 三顶层 + 全部子类型节点 + 软失败。
// model 层只有 AST 模型；io 层 SignatureReader（读）/ SignatureWriter（写）对称。

#include <gtest/gtest.h>

#include "jclassfile/io/signature_reader.hpp"
#include "jclassfile/io/signature_writer.hpp"

namespace {

using namespace jclassfile::model::signature;
using jclassfile::io::SignatureReader;
using jclassfile::io::SignatureWriter;

// 断言 read -> write round-trip 一致
void expectRoundTrip(const std::string& sig)
{
    auto ast = SignatureReader::read(sig);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->kind() != SignatureKind::Illegal ? SignatureWriter::write(*ast) : sig, sig)
        << "round-trip mismatch for: " << sig;
}

// ----------------------------------------------------------------------------
// FieldSignature
// ----------------------------------------------------------------------------

TEST(SignatureFieldTest, SimpleClassType)
{
    auto ast = SignatureReader::read("Ljava/lang/String;");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->kind(), SignatureKind::Field);
    const auto* fs = static_cast<FieldSignature*>(ast.get());
    ASSERT_NE(fs->type(), nullptr);
    EXPECT_EQ(fs->type()->kind(), SignatureTypeKind::ClassType);
    const auto* ct = static_cast<const ClassType*>(fs->type());
    EXPECT_EQ(ct->name(), "java/lang/String");
    EXPECT_TRUE(ct->typeArguments().empty());
    EXPECT_TRUE(ct->innerComponents().empty());
}

TEST(SignatureFieldTest, TypeVariable)
{
    auto ast = SignatureReader::read("TTVar;");
    ASSERT_EQ(ast->kind(), SignatureKind::Field);
    const auto* t = static_cast<FieldSignature*>(ast.get())->type();
    ASSERT_EQ(t->kind(), SignatureTypeKind::TypeVariable);
    EXPECT_EQ(static_cast<const TypeVariableType*>(t)->name(), "TVar");
}

TEST(SignatureFieldTest, GenericClassType)
{
    auto ast = SignatureReader::read("Ljava/util/List<Ljava/lang/String;>;");
    ASSERT_EQ(ast->kind(), SignatureKind::Field);
    const auto* ct = static_cast<const ClassType*>(static_cast<FieldSignature*>(ast.get())->type());
    ASSERT_EQ(ct->name(), "java/util/List");
    ASSERT_EQ(ct->typeArguments().size(), 1u);
    const auto& arg = ct->typeArguments()[0];
    EXPECT_EQ(arg.wildcard(), WildcardKind::Exact);
    ASSERT_NE(arg.type(), nullptr);
    EXPECT_EQ(arg.type()->kind(), SignatureTypeKind::ClassType);
}

TEST(SignatureFieldTest, WildcardExtends)
{
    auto        ast = SignatureReader::read("Ljava/util/List<+Ljava/lang/Number;>;");
    const auto* ct = static_cast<const ClassType*>(static_cast<FieldSignature*>(ast.get())->type());
    ASSERT_EQ(ct->typeArguments().size(), 1u);
    EXPECT_EQ(ct->typeArguments()[0].wildcard(), WildcardKind::Extends);
}

TEST(SignatureFieldTest, WildcardSuper)
{
    auto        ast = SignatureReader::read("Ljava/util/List<-Ljava/lang/Number;>;");
    const auto* ct = static_cast<const ClassType*>(static_cast<FieldSignature*>(ast.get())->type());
    EXPECT_EQ(ct->typeArguments()[0].wildcard(), WildcardKind::Super);
}

TEST(SignatureFieldTest, WildcardUnbounded)
{
    auto        ast = SignatureReader::read("Ljava/util/List<*>;");
    const auto* ct = static_cast<const ClassType*>(static_cast<FieldSignature*>(ast.get())->type());
    ASSERT_EQ(ct->typeArguments().size(), 1u);
    EXPECT_EQ(ct->typeArguments()[0].wildcard(), WildcardKind::Unbounded);
    EXPECT_EQ(ct->typeArguments()[0].type(), nullptr);
}

TEST(SignatureFieldTest, ArrayOfPrimitives)
{
    auto        ast = SignatureReader::read("[I");
    const auto* t   = static_cast<FieldSignature*>(ast.get())->type();
    ASSERT_EQ(t->kind(), SignatureTypeKind::Array);
    const auto* arr = static_cast<const ArrayType*>(t);
    EXPECT_EQ(arr->dims(), 1);
    EXPECT_EQ(arr->elementType()->kind(), SignatureTypeKind::Base);
}

TEST(SignatureFieldTest, MultiDimArrayOfObject)
{
    auto        ast = SignatureReader::read("[[Ljava/lang/Object;");
    const auto* arr =
        static_cast<const ArrayType*>(static_cast<FieldSignature*>(ast.get())->type());
    EXPECT_EQ(arr->dims(), 2);
    EXPECT_EQ(arr->elementType()->kind(), SignatureTypeKind::ClassType);
}

TEST(SignatureFieldTest, InnerClassType)
{
    auto ast =
        SignatureReader::read("Ljava/util/Map$Entry<Ljava/lang/String;Ljava/lang/Integer;>;");
    const auto* ct = static_cast<const ClassType*>(static_cast<FieldSignature*>(ast.get())->type());
    EXPECT_EQ(ct->name(), "java/util/Map$Entry");
    EXPECT_EQ(ct->typeArguments().size(), 2u);
}

TEST(SignatureFieldTest, InnerClassSuffix)
{
    // Map<TK;TV;>.Entry<TK;TV;>  — 内部类层用 '.' 分隔
    auto        ast = SignatureReader::read("Ljava/util/Map<TK;TV;>.Entry<TK;TV;>;");
    const auto* ct = static_cast<const ClassType*>(static_cast<FieldSignature*>(ast.get())->type());
    EXPECT_EQ(ct->name(), "java/util/Map");
    ASSERT_EQ(ct->innerComponents().size(), 1u);
    EXPECT_EQ(ct->innerComponents()[0].name(), "Entry");
    EXPECT_EQ(ct->innerComponents()[0].typeArguments().size(), 2u);
}

// ----------------------------------------------------------------------------
// MethodSignature
// ----------------------------------------------------------------------------

TEST(SignatureMethodTest, SimpleMethod)
{
    auto ast = SignatureReader::read("(I)V");
    ASSERT_EQ(ast->kind(), SignatureKind::Method);
    const auto* ms = static_cast<MethodSignature*>(ast.get());
    EXPECT_TRUE(ms->typeParameters().empty());
    ASSERT_EQ(ms->parameterTypes().size(), 1u);
    EXPECT_EQ(ms->parameterTypes()[0]->kind(), SignatureTypeKind::Base);
    ASSERT_NE(ms->returnType(), nullptr);
    EXPECT_EQ(ms->returnType()->kind(), SignatureTypeKind::Base);
}

TEST(SignatureMethodTest, MultipleParams)
{
    auto        ast = SignatureReader::read("(Ljava/lang/String;I[J)V");
    const auto* ms  = static_cast<MethodSignature*>(ast.get());
    ASSERT_EQ(ms->parameterTypes().size(), 3u);
    EXPECT_EQ(ms->parameterTypes()[0]->kind(), SignatureTypeKind::ClassType);
    EXPECT_EQ(ms->parameterTypes()[1]->kind(), SignatureTypeKind::Base);
    EXPECT_EQ(ms->parameterTypes()[2]->kind(), SignatureTypeKind::Array);
}

TEST(SignatureMethodTest, GenericMethodWithTypeParams)
{
    // <T:Ljava/lang/Object;>(TT;)TT;
    auto ast = SignatureReader::read("<T:Ljava/lang/Object;>(TT;)TT;");
    ASSERT_EQ(ast->kind(), SignatureKind::Method);
    const auto* ms = static_cast<MethodSignature*>(ast.get());
    ASSERT_EQ(ms->typeParameters().size(), 1u);
    const auto& tp = ms->typeParameters()[0];
    EXPECT_EQ(tp.name(), "T");
    ASSERT_NE(tp.classBound(), nullptr);
    EXPECT_EQ(tp.classBound()->kind(), SignatureTypeKind::ClassType);
    EXPECT_TRUE(tp.interfaceBounds().empty());
}

TEST(SignatureMethodTest, TypeParamWithInterfaceBound)
{
    // javac 标准形式：class bound Ljava/lang/Object; 带结尾 ';'，
    // interface bound :Ljava/lang/Comparable<TT;>;
    auto ast = SignatureReader::read("<T:Ljava/lang/Object;:Ljava/lang/Comparable<TT;>;>(TT;)I");
    const auto* ms = static_cast<MethodSignature*>(ast.get());
    ASSERT_EQ(ms->typeParameters().size(), 1u);
    ASSERT_EQ(ms->typeParameters()[0].interfaceBounds().size(), 1u);
}

TEST(SignatureMethodTest, TypeParamNoClassBound)
{
    // <T::Ljava/lang/Comparable<TT;>;>(TT;)I  — class bound 缺省（隐式 Object）
    auto        ast = SignatureReader::read("<T::Ljava/lang/Comparable<TT;>;>(TT;)I");
    const auto* ms  = static_cast<MethodSignature*>(ast.get());
    ASSERT_EQ(ms->typeParameters().size(), 1u);
    EXPECT_EQ(ms->typeParameters()[0].classBound(), nullptr);   // 缺省 class bound
}

TEST(SignatureMethodTest, ThrowsClause)
{
    // ()V^Ljava/lang/Throwable;^Ljava/lang/Exception;
    auto        ast = SignatureReader::read("()V^Ljava/lang/Throwable;^Ljava/lang/Exception;");
    const auto* ms  = static_cast<MethodSignature*>(ast.get());
    ASSERT_EQ(ms->throwsTypes().size(), 2u);
}

// ----------------------------------------------------------------------------
// ClassSignature
// ----------------------------------------------------------------------------

TEST(SignatureClassTest, SimpleClass)
{
    // java/lang/Object 无泛型：superclass + 无 interface
    auto ast = SignatureReader::read("Ljava/lang/Object;Ljava/lang/Serializable;");
    ASSERT_EQ(ast->kind(), SignatureKind::Class);
    const auto* cs = static_cast<ClassSignature*>(ast.get());
    EXPECT_TRUE(cs->typeParameters().empty());
    ASSERT_NE(cs->superclass(), nullptr);
    EXPECT_EQ(cs->superinterfaces().size(), 1u);
}

TEST(SignatureClassTest, GenericClassWithSuperinterface)
{
    // <K:Ljava/lang/Object;V:Ljava/lang/Object;>Ljava/lang/Object;Ljava/util/Map<TK;TV;>;
    auto ast = SignatureReader::read(
        "<K:Ljava/lang/Object;V:Ljava/lang/Object;>Ljava/lang/Object;Ljava/util/Map<TK;TV;>;");
    ASSERT_EQ(ast->kind(), SignatureKind::Class);
    const auto* cs = static_cast<ClassSignature*>(ast.get());
    ASSERT_EQ(cs->typeParameters().size(), 2u);
    EXPECT_EQ(cs->typeParameters()[0].name(), "K");
    EXPECT_EQ(cs->typeParameters()[1].name(), "V");
    ASSERT_EQ(cs->superinterfaces().size(), 1u);
}

// ----------------------------------------------------------------------------
// Round-trip：parse -> toString 必须等于原串
// ----------------------------------------------------------------------------

TEST(SignatureRoundTripTest, FieldSignatures)
{
    expectRoundTrip("Ljava/lang/String;");
    expectRoundTrip("TTVar;");
    expectRoundTrip("[I");
    expectRoundTrip("[[Ljava/lang/Object;");
    expectRoundTrip("Ljava/util/List<Ljava/lang/String;>;");
    expectRoundTrip("Ljava/util/Map<Ljava/lang/String;Ljava/lang/Integer;>;");
    expectRoundTrip("Ljava/util/List<*>;");
    expectRoundTrip("Ljava/util/List<+Ljava/lang/Number;>;");
    expectRoundTrip("Ljava/util/List<-Ljava/lang/Number;>;");
    expectRoundTrip("Ljava/util/Map<TK;TV;>.Entry<TK;TV;>;");
    expectRoundTrip("[Ljava/util/List<[Ljava/lang/Object;>;");   // 嵌套：List<Object[]>[]
}

TEST(SignatureRoundTripTest, MethodSignatures)
{
    expectRoundTrip("(I)V");
    expectRoundTrip("()V");
    expectRoundTrip("(Ljava/lang/String;I[J)V");
    expectRoundTrip("<T:Ljava/lang/Object;>(TT;)TT;");
    expectRoundTrip("<T:Ljava/lang/Object;:Ljava/lang/Comparable<TT;>;>(TT;)I");
    expectRoundTrip("<T::Ljava/lang/Comparable<TT;>;>(TT;)I");
    expectRoundTrip("()V^Ljava/lang/Throwable;^Ljava/lang/Exception;");
    expectRoundTrip("<K:Ljava/lang/Object;V:Ljava/lang/Object;>(TK;TV;)V");
}

TEST(SignatureRoundTripTest, ClassSignatures)
{
    expectRoundTrip("Ljava/lang/Object;Ljava/lang/Serializable;");
    expectRoundTrip(
        "<K:Ljava/lang/Object;V:Ljava/lang/Object;>Ljava/lang/Object;Ljava/util/Map<TK;TV;>;");
    expectRoundTrip("<T:Ljava/lang/Object;>Ljava/lang/Object;Ljava/lang/Comparable<TT;>;Ljava/io/"
                    "Serializable;");
}

// ----------------------------------------------------------------------------
// 软失败：畸形签名不抛异常，round-trip 保留原串
// ----------------------------------------------------------------------------

TEST(SignatureIllegalTest, EmptyString)
{
    auto ast = SignatureReader::read("");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->kind(), SignatureKind::Illegal);
    EXPECT_EQ(static_cast<IllegalSignature*>(ast.get())->raw(), "");
    EXPECT_EQ(SignatureWriter::write(*ast), "");
}

TEST(SignatureIllegalTest, GarbageString)
{
    auto ast = SignatureReader::read("!!!garbage!!!");
    ASSERT_EQ(ast->kind(), SignatureKind::Illegal);
    EXPECT_EQ(SignatureWriter::write(*ast), "!!!garbage!!!");
}

TEST(SignatureIllegalTest, IncompleteClassType)
{
    // 缺少结尾 ';' 的类类型
    auto ast = SignatureReader::read("Ljava/lang/String");
    EXPECT_EQ(SignatureWriter::write(*ast), "Ljava/lang/String");
}

TEST(SignatureIllegalTest, UnbalancedAngleBracket)
{
    auto ast = SignatureReader::read("Ljava/util/List<Ljava/lang/String;");
    // 无法完整解析 -> IllegalSignature，round-trip 保留原串
    EXPECT_EQ(SignatureWriter::write(*ast), "Ljava/util/List<Ljava/lang/String;");
}

TEST(SignatureIllegalTest, NeverThrows)
{
    // 各种畸形输入都不应抛异常
    EXPECT_NO_THROW({ (void)SignatureReader::read("[[["); });
    EXPECT_NO_THROW({ (void)SignatureReader::read("L"); });
    EXPECT_NO_THROW({ (void)SignatureReader::read("<"); });
    EXPECT_NO_THROW({ (void)SignatureReader::read("(("); });
    EXPECT_NO_THROW({ (void)SignatureReader::read(":::"); });
    EXPECT_NO_THROW({ (void)SignatureReader::read("X"); });   // 不合法的首字符
}

// ----------------------------------------------------------------------------
// 构建能力：手动构造 AST 并序列化（验证可编程构造 + 修改路径）
// ----------------------------------------------------------------------------

TEST(SignatureBuilderTest, BuildFieldSignature)
{
    // 手动构造 List<String>
    std::vector<TypeArgument> args;
    args.emplace_back(WildcardKind::Exact,
                      std::make_unique<ClassType>("java/lang/String", std::vector<TypeArgument>{}));
    auto           type = std::make_unique<ClassType>("java/util/List", std::move(args));
    FieldSignature fs(std::move(type));

    EXPECT_EQ(SignatureWriter::write(fs), "Ljava/util/List<Ljava/lang/String;>;");
}

TEST(SignatureBuilderTest, BuildMethodSignature)
{
    // (I)V
    auto                                        ret = std::make_unique<BaseType>('V');
    std::vector<std::unique_ptr<SignatureType>> params;
    params.push_back(std::make_unique<BaseType>('I'));
    MethodSignature ms({}, std::move(params), std::move(ret), {});

    EXPECT_EQ(SignatureWriter::write(ms), "(I)V");
}

}   // namespace
