// 阶段 4b：ClassFileReader 端到端 + ClassBuilder + AttributeReader stub 测试
// 覆盖：
//   - ClassBuilder 基本行为（version/access/thisClass/superClass/add* + build）
//   - ClassFileReader 错误路径：bad magic、tag mismatch
//   - ClassFileReader 最小可解析 class：UTF8 + Class + Class + this/super 引用
//   - ClassFileReader 2-pass pool 处理 forward reference（CLASS #5 → UTF8 #6）
//   - AttributeReader stub：name + length + raw bytes → DefaultAttribute
//   - ClassFileReader 字段/方法读取（access/name/desc/attributes）
//   - 5 config flag 读写

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "jclassfile/io/attribute_reader.hpp"
#include "jclassfile/io/class_builder.hpp"
#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/io/indexable_byte_stream.hpp"
#include "jclassfile/invalid_class_exception.hpp"
#include "libca/core/datatype.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_method_handle.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/attribute/bootstrap_methods_attribute.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/record_attribute.hpp"
#include "jclassfile/model/field.hpp"
#include "jclassfile/model/method.hpp"
#include "jclassfile/transform/illegal_stripping_transformer.hpp"
#include "test_helper.hpp"

using namespace jclassfile;
using namespace jclassfile::io;
using namespace jclassfile::model;

namespace {

// ============== 字节构造助手 ==============

void appendInt(std::vector<ca::u8>& buf, ca::u32 v)
{
    buf.push_back(static_cast<ca::u8>((v >> 24) & 0xFF));
    buf.push_back(static_cast<ca::u8>((v >> 16) & 0xFF));
    buf.push_back(static_cast<ca::u8>((v >> 8) & 0xFF));
    buf.push_back(static_cast<ca::u8>(v & 0xFF));
}

void appendShort(std::vector<ca::u8>& buf, ca::u16 v)
{
    buf.push_back(static_cast<ca::u8>((v >> 8) & 0xFF));
    buf.push_back(static_cast<ca::u8>(v & 0xFF));
}

void appendUtf8Entry(std::vector<ca::u8>& buf, const std::string& s)
{
    buf.push_back(static_cast<ca::u8>(CpTag::UTF8));
    ca::u16 len = static_cast<ca::u16>(s.size());
    appendShort(buf, len);
    for (char c : s) buf.push_back(static_cast<ca::u8>(c));
}

void appendClassEntry(std::vector<ca::u8>& buf, ca::u16 utf8Index)
{
    buf.push_back(static_cast<ca::u8>(CpTag::CLASS));
    appendShort(buf, utf8Index);
}

void appendNameTypeEntry(std::vector<ca::u8>& buf, ca::u16 nameIdx, ca::u16 descIdx)
{
    buf.push_back(static_cast<ca::u8>(CpTag::NAME_TYPE));
    appendShort(buf, nameIdx);
    appendShort(buf, descIdx);
}

void appendIntegerEntry(std::vector<ca::u8>& buf, ca::u32 value)
{
    buf.push_back(static_cast<ca::u8>(CpTag::INTEGER));
    appendInt(buf, value);
}

// 构造一个最小可用的 class：this="Foo", super="Bar", 0 fields/0 methods/0 attrs
std::vector<ca::u8> buildMinimalClass()
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);   // magic
    appendShort(buf, 0);          // minor
    appendShort(buf, 52);         // major = JAVA8

    // Pool layout:
    //   #1  Utf8 "Foo"
    //   #2  Utf8 "Bar"
    //   #3  Class -> #1   (this class)
    //   #4  Class -> #2   (super class)
    //   numConstants = 5
    appendShort(buf, 5);
    appendUtf8Entry(buf, "Foo");
    appendUtf8Entry(buf, "Bar");
    appendClassEntry(buf, 1);
    appendClassEntry(buf, 2);

    appendShort(buf, 0x0021);   // ACC_PUBLIC | ACC_SUPER
    appendShort(buf, 3);        // this_class = #3
    appendShort(buf, 4);        // super_class = #4
    appendShort(buf, 0);        // 0 interfaces
    appendShort(buf, 0);        // 0 fields
    appendShort(buf, 0);        // 0 methods
    appendShort(buf, 0);        // 0 class attributes
    return buf;
}

// 构造 forward-reference class：Utf8 #1 引用 Class #2（指向 #1）
// 实际用于验证 2-pass pool reading 正确解析 forward reference
std::vector<ca::u8> buildForwardRefClass()
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);

    // Pool: #1 Utf8 "X"  #2 Class -> #1
    // numConstants = 3
    appendShort(buf, 3);
    appendUtf8Entry(buf, "X");
    appendClassEntry(buf, 1);

    appendShort(buf, 0x0021);
    appendShort(buf, 2);   // this = #2
    appendShort(buf, 0);   // super = 0 (invalid, but we accept 0 here)
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 0);
    return buf;
}

// 构造 1 field + 1 method 的 class
std::vector<ca::u8> buildFieldMethodClass()
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);

    // Pool: #1 Utf8 "Foo"  #2 Utf8 "Bar"  #3 Utf8 "count"  #4 Utf8 "I"
    //       #5 Class -> #1 (this)  #6 Class -> #2 (super)
    //       #7 Utf8 "increment"  #8 Utf8 "()V"
    //       #9 NameType(#3, #4)    #10 NameType(#7, #8)  -- not strictly needed for fields/methods
    // numConstants = 6 (we only need 1-6 for fields/methods/this/super)
    // Actually, for field with no attributes, we need: name_idx, desc_idx pointing to Utf8.
    // So pool = #1 "Foo" #2 "Bar" #3 "count" #4 "I" #5 Class(1) #6 Class(2) #7 "increment" #8 "()V"
    //   numConstants = 9
    appendShort(buf, 9);
    appendUtf8Entry(buf, "Foo");
    appendUtf8Entry(buf, "Bar");
    appendUtf8Entry(buf, "count");
    appendUtf8Entry(buf, "I");
    appendClassEntry(buf, 1);
    appendClassEntry(buf, 2);
    appendUtf8Entry(buf, "increment");
    appendUtf8Entry(buf, "()V");

    appendShort(buf, 0x0021);   // ACC_PUBLIC | ACC_SUPER
    appendShort(buf, 5);        // this = #5
    appendShort(buf, 6);        // super = #6
    appendShort(buf, 0);        // 0 interfaces

    // 1 field: "count" (Utf8 #3) type "I" (Utf8 #4), 0 attrs
    appendShort(buf, 1);
    appendShort(buf, 0x0002);   // ACC_PRIVATE
    appendShort(buf, 3);        // name = #3
    appendShort(buf, 4);        // desc = #4
    appendShort(buf, 0);        // 0 attrs

    // 1 method: "increment" (Utf8 #7) type "()V" (Utf8 #8), 0 attrs
    appendShort(buf, 1);
    appendShort(buf, 0x0001);   // ACC_PUBLIC
    appendShort(buf, 7);
    appendShort(buf, 8);
    appendShort(buf, 0);

    appendShort(buf, 0);   // 0 class attributes
    return buf;
}

// 构造 1 class attribute (DefaultAttribute) 的 class
std::vector<ca::u8> buildClassWithAttribute()
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);

    // Pool: #1 Utf8 "Foo"  #2 Utf8 "Bar"  #3 Class(1)  #4 Class(2)
    //       #5 Utf8 "Custom"  (attribute name)
    // numConstants = 6
    appendShort(buf, 6);
    appendUtf8Entry(buf, "Foo");
    appendUtf8Entry(buf, "Bar");
    appendClassEntry(buf, 1);
    appendClassEntry(buf, 2);
    appendUtf8Entry(buf, "Custom");

    appendShort(buf, 0x0021);
    appendShort(buf, 3);
    appendShort(buf, 4);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 1);   // 1 class attribute

    // Attribute: name_index = 5 ("Custom"), length = 4, data = {0xDE,0xAD,0xBE,0xEF}
    appendShort(buf, 5);
    appendInt(buf, 4);
    buf.push_back(0xDE);
    buf.push_back(0xAD);
    buf.push_back(0xBE);
    buf.push_back(0xEF);
    return buf;
}

std::vector<ca::u8> buildClassWithIntAnnotationIndex(ca::u16 constValueIndex)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);

    appendShort(buf, 9);
    appendUtf8Entry(buf, "P0MalformedAnnotation");       // #1
    appendClassEntry(buf, 1);                            // #2
    appendUtf8Entry(buf, "java/lang/Object");            // #3
    appendClassEntry(buf, 3);                            // #4
    appendUtf8Entry(buf, "RuntimeVisibleAnnotations");   // #5
    appendUtf8Entry(buf, "LA;");                         // #6
    appendUtf8Entry(buf, "value");                       // #7
    appendIntegerEntry(buf, 1);                          // #8

    appendShort(buf, 0x0021);
    appendShort(buf, 2);   // this_class
    appendShort(buf, 4);   // super_class
    appendShort(buf, 0);   // interfaces_count
    appendShort(buf, 0);   // fields_count
    appendShort(buf, 0);   // methods_count
    appendShort(buf, 1);   // attributes_count
    appendShort(buf, 5);   // RuntimeVisibleAnnotations
    appendInt(buf, 11);    // attribute_length
    appendShort(buf, 1);   // num_annotations
    appendShort(buf, 6);   // annotation type_index
    appendShort(buf, 1);   // num_element_value_pairs
    appendShort(buf, 7);   // element_name_index
    buf.push_back(static_cast<ca::u8>('I'));
    appendShort(buf, constValueIndex);
    return buf;
}

// 构造 1 个方法（带 Code 属性）的 class。
// codeLengthRaw 为写入 u4 code_length 字段的原始值（可为 0/负/超界）；
// codeBytes 为实际 code 数组字节；exceptionCount + exceptionBytes 为异常表。
// Pool: #1 "Foo" #2 "java/lang/Object" #3 Class(2) #4 Class(1)
//       #5 "Code" #6 "run" #7 "()V"，numConstants = 8
std::vector<ca::u8> buildClassWithCode(ca::u32                        codeLengthRaw,
                                       const std::vector<ca::u8>&     codeBytes,
                                       ca::u16                        exceptionCount,
                                       const std::vector<ca::u8>&     exceptionBytes)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);

    appendShort(buf, 8);
    appendUtf8Entry(buf, "Foo");
    appendUtf8Entry(buf, "java/lang/Object");
    appendClassEntry(buf, 2);
    appendClassEntry(buf, 1);
    appendUtf8Entry(buf, "Code");
    appendUtf8Entry(buf, "run");
    appendUtf8Entry(buf, "()V");

    appendShort(buf, 0x0021);
    appendShort(buf, 4);   // this = #4
    appendShort(buf, 3);   // super = #3
    appendShort(buf, 0);   // interfaces
    appendShort(buf, 0);   // fields

    // 1 method: access / name=#6 / desc=#7 / 1 attribute
    appendShort(buf, 1);
    appendShort(buf, 0x0001);
    appendShort(buf, 6);
    appendShort(buf, 7);
    appendShort(buf, 1);

    // Code attribute: max_stack + max_locals + code_length + code
    //               + exception_table_length + exceptions + attributes_count
    ca::u32 body = 2 + 2 + 4 + static_cast<ca::u32>(codeBytes.size()) + 2 +
                   static_cast<ca::u32>(exceptionBytes.size()) + 2;
    appendShort(buf, 5);
    appendInt(buf, body);
    appendShort(buf, 1);   // max_stack
    appendShort(buf, 1);   // max_locals
    appendInt(buf, codeLengthRaw);
    for (ca::u8 b : codeBytes) buf.push_back(b);
    appendShort(buf, exceptionCount);
    for (ca::u8 b : exceptionBytes) buf.push_back(b);
    appendShort(buf, 0);   // code 内 attributes_count

    appendShort(buf, 0);   // 0 class attributes
    return buf;
}

const CodeAttribute* findCodeAttribute(const Method* m)
{
    for (const Attribute* a : m->attributes()) {
        if (a->name().text() == "Code") return static_cast<const CodeAttribute*>(a);
    }
    return nullptr;
}

// 追加一个 u2 start_pc / u2 end_pc / u2 handler_pc / u2 catch_type 的异常表条目。
void appendExceptionEntry(std::vector<ca::u8>& buf, ca::u16 startPc, ca::u16 endPc, ca::u16 handlerPc,
                          ca::u16 catchType)
{
    appendShort(buf, startPc);
    appendShort(buf, endPc);
    appendShort(buf, handlerPc);
    appendShort(buf, catchType);
}

// 构造 record class：1 个 record component "x"，component 上挂 1 条
// RuntimeVisibleTypeAnnotations，target_type 由参数指定。
// Pool: #1 "Rec" #2 "java/lang/Object" #3 Class(2) #4 Class(1) #5 "Record"
//       #6 "x" #7 "I" #8 "RuntimeVisibleTypeAnnotations" #9 "LA;"，numConstants = 10
std::vector<ca::u8> buildRecordWithComponentTypeAnnotation(int targetType)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 61);   // JAVA17：RECORD（58+）之后

    appendShort(buf, 10);
    appendUtf8Entry(buf, "Rec");
    appendUtf8Entry(buf, "java/lang/Object");
    appendClassEntry(buf, 2);
    appendClassEntry(buf, 1);
    appendUtf8Entry(buf, "Record");
    appendUtf8Entry(buf, "x");
    appendUtf8Entry(buf, "I");
    appendUtf8Entry(buf, "RuntimeVisibleTypeAnnotations");
    appendUtf8Entry(buf, "LA;");

    appendShort(buf, 0x0031);
    appendShort(buf, 4);   // this = #4
    appendShort(buf, 3);   // super = #3
    appendShort(buf, 0);   // interfaces
    appendShort(buf, 0);   // fields
    appendShort(buf, 0);   // methods

    // 1 class attribute: Record
    // body = u2 components_count
    //      + component(u2 name + u2 desc + u2 attrs_count + attr(2+4+body))
    //      = 2 + (6 + 2 + 4 + 8) = 22
    appendShort(buf, 1);
    appendShort(buf, 5);
    appendInt(buf, 22);
    appendShort(buf, 1);   // 1 component
    appendShort(buf, 6);   // name "x"
    appendShort(buf, 7);   // desc "I"
    appendShort(buf, 1);   // 1 component attribute
    appendShort(buf, 8);   // "RuntimeVisibleTypeAnnotations"
    appendInt(buf, 8);     // u2 num_annotations + type_annotation(1+1+2+2)
    appendShort(buf, 1);   // 1 annotation
    buf.push_back(static_cast<ca::u8>(targetType));   // target_type
    buf.push_back(0);      // type_path_length
    appendShort(buf, 9);   // type_index "LA;"
    appendShort(buf, 0);   // num_element_value_pairs

    return buf;
}

}   // namespace

// ============== ClassBuilder 测试 ==============

TEST(ClassBuilderTest, DefaultState)
{
    ClassBuilder b;
    EXPECT_EQ(b.getVersionMajor(), 0);
    EXPECT_EQ(b.getVersionMinor(), 0);
    EXPECT_EQ(b.getAccess(), 0);
    EXPECT_TRUE(b.getInterfaces().empty());
    EXPECT_TRUE(b.getFields().empty());
    EXPECT_TRUE(b.getMethods().empty());
    EXPECT_TRUE(b.getAttributes().empty());
}

TEST(ClassBuilderTest, IsOakVersionPreJava1)
{
    ClassBuilder b;
    b.setVersionMajor(45);
    b.setVersionMinor(2);
    EXPECT_TRUE(b.isOakVersion());
    b.setVersionMinor(0);
    EXPECT_TRUE(b.isOakVersion());
    b.setVersionMinor(3);
    EXPECT_FALSE(b.isOakVersion());
    b.setVersionMajor(44);
    EXPECT_TRUE(b.isOakVersion());
}

TEST(ClassBuilderTest, IsAnnotationAndModule)
{
    ClassBuilder b;
    b.setAccess(0x2000);   // ACC_ANNOTATION
    EXPECT_TRUE(b.isAnnotation());
    EXPECT_FALSE(b.isModule());

    b.setAccess(0x8000);   // ACC_MODULE
    EXPECT_FALSE(b.isAnnotation());
    EXPECT_TRUE(b.isModule());
}

TEST(ClassBuilderTest, SettersAndGetters)
{
    ClassBuilder b;
    b.setVersionMajor(61);
    b.setVersionMinor(1);
    b.setAccess(0x0001);
    CpUtf8  name("X");
    CpClass cls(&name);
    b.setThisClass(&cls);
    b.setSuperClass(&cls);
    b.addInterface(&cls);
    b.addField(nullptr);
    b.addMethod(nullptr);
    b.addAttribute(nullptr);
    EXPECT_EQ(b.getVersionMajor(), 61);
    EXPECT_EQ(b.getVersionMinor(), 1);
    EXPECT_EQ(b.getAccess(), 0x0001);
    EXPECT_EQ(b.getThisClass(), &cls);
    EXPECT_EQ(b.getSuperClass(), &cls);
    EXPECT_EQ(b.getInterfaces().size(), 1u);
    EXPECT_EQ(b.getFields().size(), 1u);
    EXPECT_EQ(b.getMethods().size(), 1u);
    EXPECT_EQ(b.getAttributes().size(), 1u);
}

// ============== ClassFileReader 错误路径 ==============

TEST(ClassFileReaderTest, BadMagicThrows)
{
    std::vector<ca::u8> buf = {0x00, 0x00, 0x00, 0x00, 0, 0, 0, 0};
    ClassFileReader     r;
    EXPECT_THROW(r.read(buf), InvalidClassException);
}

TEST(ClassFileReaderTest, UnknownPoolTagThrows)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);
    appendShort(buf, 2);   // numConstants = 2 (only #1)
    buf.push_back(0xFF);   // unknown tag
    ClassFileReader r;
    EXPECT_THROW(r.read(buf), InvalidClassException);
}

TEST(ClassFileReaderTest, InternalPoolTag0Throws)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);
    appendShort(buf, 2);   // numConstants = 2 (only #1)
    buf.push_back(0x00);   // CpTag::INTERNAL = 0
    buf.push_back(0x00);   // data byte
    ClassFileReader r;
    EXPECT_THROW(r.read(buf), InvalidClassException);
}

TEST(ClassFileReaderTest, SkipUnknownPoolTagTolerant)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);
    appendShort(buf, 2);   // numConstants = 2 (only #1)
    buf.push_back(0xFF);   // unknown tag
    ClassFileReader r;
    r.setSkipUnknownPoolTags(true);
    // 跳过模式下 1 个未知 tag 后不会崩溃，但 class 不完整，
    // 后续读 this/super 会因 CP 索引失效而失败
    // 关键是 read() 不因 InvalidClassException("Unknown constant-pool tag") 崩溃
    EXPECT_THROW(r.read(buf), InvalidClassException);
}

// ============== ClassFileReader 正常路径 ==============

TEST(ClassFileReaderTest, ReadsMinimalClass)
{
    ClassFileReader r;
    auto            cf = r.read(buildMinimalClass());
    EXPECT_EQ(cf.getName(), "Foo");
    EXPECT_EQ(cf.getSuperName(), "Bar");
    EXPECT_EQ(cf.getVersionMajor(), 52);
    EXPECT_EQ(cf.getVersionMinor(), 0);
    EXPECT_EQ(cf.getAccess(), 0x0021);
    EXPECT_EQ(cf.getInterfaceClasses().size(), 0u);
    EXPECT_EQ(cf.getFields().size(), 0u);
    EXPECT_EQ(cf.getMethods().size(), 0u);
    EXPECT_EQ(cf.attributes().size(), 0u);
    EXPECT_EQ(cf.holderType(), AttributeHolderType::CLASS);
}

TEST(ClassFileReaderTest, HandlesForwardReferenceInPool)
{
    ClassFileReader r;
    // this = #2 (Class → #1 "X"), super = 0 (not a valid class but reader tolerates)
    auto cf = r.read(buildForwardRefClass());
    EXPECT_EQ(cf.getName(), "X");
    EXPECT_EQ(cf.getSuperName(), "");
}

TEST(ClassFileReaderTest, ReadsFieldAndMethod)
{
    ClassFileReader r;
    auto            cf = r.read(buildFieldMethodClass());
    EXPECT_EQ(cf.getName(), "Foo");
    EXPECT_EQ(cf.getSuperName(), "Bar");
    ASSERT_EQ(cf.getFields().size(), 1u);
    ASSERT_EQ(cf.getMethods().size(), 1u);

    const Field* f = cf.getFields()[0];
    EXPECT_EQ(f->getAccess(), 0x0002);
    EXPECT_EQ(f->name()->text(), "count");
    EXPECT_EQ(f->type()->text(), "I");

    const Method* m = cf.getMethods()[0];
    EXPECT_EQ(m->getAccess(), 0x0001);
    EXPECT_EQ(m->name()->text(), "increment");
    EXPECT_EQ(m->type()->text(), "()V");
}

TEST(ClassFileReaderTest, ReadsClassAttributeAsDefault)
{
    ClassFileReader r;
    auto            cf = r.read(buildClassWithAttribute());
    ASSERT_EQ(cf.attributes().size(), 1u);
    const Attribute* a = cf.attributes()[0];
    EXPECT_EQ(a->name().text(), "Custom");
    // 项目禁用 RTTI；DefaultAttribute 阶段 4c 是唯一可构造的 attribute，
    // 用 computeInternalLength 间接验证 data 长度 = 4
    EXPECT_EQ(a->computeInternalLength(), 4);
}

namespace {

// 复用 buildClassWithAttribute 的骨架，但把 attribute_length 换成敌意值。
std::vector<ca::u8> buildClassWithHostileAttributeLength(ca::u32 length)
{
    auto buf = buildClassWithAttribute();
    // attribute_length 是倒数第 8..5 字节（4 数据字节 + 2 name_index + 4 length 头）
    auto  pos = buf.size() - 8;
    auto   be = length;
    for (int i = 0; i < 4; i++) {
        buf[pos + i] = static_cast<ca::u8>((be >> (24 - 8 * i)) & 0xFF);
    }
    return buf;
}

}   // namespace

// 敌意 attribute_length ≥ 0x80000000：static_cast<int> 后为负，旧实现里负长度
// 产生反转 slice 与负绝对位置，require() 只查上界不查负，最终负下标读（UB）。
// 宽容模式（默认 dropEofAttributes）：属性被丢弃，解析不产生 UB；严格模式抛出。
TEST(ClassFileReaderTest, HostileNegativeAttributeLengthThrows)
{
    ClassFileReader lenient;
    auto            cf = lenient.read(buildClassWithHostileAttributeLength(0x80000000u));
    EXPECT_TRUE(cf.attributes().empty());

    ClassFileReader strict;
    strict.setDropEofAttributes(false);
    EXPECT_THROW(strict.read(buildClassWithHostileAttributeLength(0x80000000u)),
                 InvalidClassException);
    EXPECT_THROW(strict.read(buildClassWithHostileAttributeLength(0xFFFFFFFFu)),
                 InvalidClassException);
}

// 敌意 attribute_length = INT_MAX：contentStart + length 溢出 int；旧实现溢出后
// 结果不定，修复后钳到 buffer 上界并以 out_of_range 转 InvalidClassException 报告。
TEST(ClassFileReaderTest, HostileOverflowingAttributeLengthThrows)
{
    ClassFileReader lenient;
    auto            cf = lenient.read(buildClassWithHostileAttributeLength(0x7FFFFFFFu));
    EXPECT_TRUE(cf.attributes().empty());

    ClassFileReader strict;
    strict.setDropEofAttributes(false);
    EXPECT_THROW(strict.read(buildClassWithHostileAttributeLength(0x7FFFFFFFu)),
                 InvalidClassException);
    EXPECT_THROW(strict.read(buildClassWithHostileAttributeLength(0x70000000u)),
                 InvalidClassException);
}

namespace {

// RuntimeVisibleAnnotations 注解值：深度嵌套数组（tag '[' × depth，最内层 I 常量）。
// 每层 3 字节（tag + u16 count=1），几 MB 即可造百万级递归深度。
std::vector<ca::u8> buildClassWithDeeplyNestedAnnotationArray(int depth)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);

    // #1 Utf8 "Foo" #2 Utf8 "java/lang/Object" #3 Class(1) #4 Class(2)
    // #5 Utf8 "RuntimeVisibleAnnotations" #6 Utf8 "LA;" #7 Utf8 "value" #8 Integer(1)
    appendShort(buf, 9);
    appendUtf8Entry(buf, "Foo");
    appendUtf8Entry(buf, "java/lang/Object");
    appendClassEntry(buf, 1);
    appendClassEntry(buf, 2);
    appendUtf8Entry(buf, "RuntimeVisibleAnnotations");
    appendUtf8Entry(buf, "LA;");
    appendUtf8Entry(buf, "value");
    appendIntegerEntry(buf, 1);

    appendShort(buf, 0x0021);
    appendShort(buf, 3);   // this
    appendShort(buf, 4);   // super
    appendShort(buf, 0);
    appendShort(buf, 0);   // fields
    appendShort(buf, 0);   // methods
    appendShort(buf, 1);   // 1 class attribute

    // 求注解体字节数：u16 num_annotations + u16 type_index + u16 num_pairs
    // + u16 name_index + depth × (tag + u16) + tag 'I' + u16 const_index
    int body = 2 + 2 + 2 + 2 + depth * 3 + 1 + 2;
    appendShort(buf, 5);   // attribute_name_index
    appendInt(buf, static_cast<ca::u32>(body));
    appendShort(buf, 1);   // num_annotations
    appendShort(buf, 6);   // type_index
    appendShort(buf, 1);   // num_element_value_pairs
    appendShort(buf, 7);   // element_name_index
    for (int i = 0; i < depth; i++) {
        buf.push_back(static_cast<ca::u8>('['));
        appendShort(buf, 1);   // num_values
    }
    buf.push_back(static_cast<ca::u8>('I'));
    appendShort(buf, 8);   // const_value_index
    return buf;
}

}   // namespace

// 深度嵌套注解数组：MAX_NESTING=50，纯数组递归边不经过 readElementPairs，
// 旧实现的深度检查被完全绕过，百万级深度直接打爆栈（栈溢出无法被 catch）。
// 修复后宽容模式丢属性、严格模式抛 InvalidClassException。
TEST(ClassFileReaderTest, DeeplyNestedAnnotationArrayHitsDepthLimit)
{
    ClassFileReader lenient;
    auto            cf = lenient.read(buildClassWithDeeplyNestedAnnotationArray(200));
    EXPECT_TRUE(cf.attributes().empty());

    ClassFileReader strict;
    strict.setDropEofAttributes(false);
    EXPECT_THROW(strict.read(buildClassWithDeeplyNestedAnnotationArray(200)),
                 InvalidClassException);
}

// 深度上限内的合法嵌套必须照常解析（MAX_NESTING=50）。
TEST(ClassFileReaderTest, ModeratelyNestedAnnotationArrayParses)
{
    ClassFileReader r;
    auto            cf = r.read(buildClassWithDeeplyNestedAnnotationArray(10));
    ASSERT_EQ(cf.attributes().size(), 1u);
    EXPECT_EQ(cf.attributes()[0]->name().text(), "RuntimeVisibleAnnotations");
}

// 解析树所有权：Field/Method/Attribute/Instruction 全部由 ClassFile 的解析
// arena 拥有并随其析构释放（此前 reader 裸 new、model 非拥有裸指针、消费方
// 从不 delete，每解析一个类泄一棵整树）。
TEST(ClassFileReaderTest, ParsedTreeIsOwnedByClassFileArena)
{
    ClassFileReader r;
    auto            cf = r.read(buildFieldMethodClass());
    EXPECT_EQ(cf.getFields().size(), 1u);
    EXPECT_EQ(cf.getMethods().size(), 1u);
    EXPECT_EQ(cf.ownedNodeCount(), 2u);   // 1 Field + 1 Method

    auto cf2 = r.read(buildClassWithAttribute());
    EXPECT_EQ(cf2.attributes().size(), 1u);
    EXPECT_EQ(cf2.ownedNodeCount(), 1u);   // 1 DefaultAttribute
}

TEST(ClassFileReaderTest, ReadsValidRuntimeVisibleAnnotation)
{
    ClassFileReader r;
    auto            cf = r.read(buildClassWithIntAnnotationIndex(8));
    ASSERT_EQ(cf.attributes().size(), 1u);
    EXPECT_EQ(cf.attributes()[0]->name().text(), "RuntimeVisibleAnnotations");
}

TEST(ClassFileReaderTest, KeepsInvalidAnnotationValueForTransformer)
{
    ClassFileReader r;
    auto            cf = r.read(buildClassWithIntAnnotationIndex(999));
    ASSERT_EQ(cf.attributes().size(), 1u);

    transform::IllegalStrippingTransformer(cf).transform();
    EXPECT_TRUE(cf.attributes().empty());
}

TEST(ClassFileReaderTest, RepairsNestedAnnotationWithInvalidConstantPoolIndexResource)
{
    // 从 issue #456 的 CrackMe 样本提取，文件名直接描述其畸形原因。
    auto bytes = test::getResource(
        "obfuscated/malformed-annotation/nested_annotation_invalid_const_value_index.class");
    ASSERT_FALSE(bytes.empty());

    ClassFileReader r;
    auto            cf = r.read(bytes);
    ASSERT_EQ(cf.attributes().size(), 1u);

    transform::IllegalStrippingTransformer(cf).transform();
    auto output  = ClassFileWriter().write(cf);
    auto cleaned = r.read(output);
    EXPECT_FALSE(cleaned.getName().empty());
}

// ---- code_length 边界（上游 3e9af780）----

// code_length == 0：JVMS 4.7.3 要求 code 数组非空。旧实现只查 > 65536，0 漏过。
TEST(ClassFileReaderTest, ZeroCodeLengthIsRejected)
{
    ClassFileReader lenient;
    auto            cf = lenient.read(buildClassWithCode(0, {}, 0, {}));
    ASSERT_EQ(cf.getMethods().size(), 1u);
    EXPECT_EQ(findCodeAttribute(cf.getMethods()[0]), nullptr);

    ClassFileReader strict;
    strict.setDropEofAttributes(false);
    EXPECT_THROW(strict.read(buildClassWithCode(0, {}, 0, {})), InvalidClassException);
}

// code_length 为负：u4 字段以 readInt() 读回可为负（如 0xFFFFFFFF → -1），
// 旧实现负值绕过 > 65536 检查。
TEST(ClassFileReaderTest, NegativeCodeLengthIsRejected)
{
    ClassFileReader lenient;
    auto            cf = lenient.read(buildClassWithCode(0xFFFFFFFFu, {}, 0, {}));
    ASSERT_EQ(cf.getMethods().size(), 1u);
    EXPECT_EQ(findCodeAttribute(cf.getMethods()[0]), nullptr);

    ClassFileReader strict;
    strict.setDropEofAttributes(false);
    EXPECT_THROW(strict.read(buildClassWithCode(0xFFFFFFFFu, {}, 0, {})), InvalidClassException);
}

// 边界语义：65536 拒绝（旧实现 > 65536 放行了 65536），65535 接受。
TEST(ClassFileReaderTest, CodeLengthBoundaryAt65536)
{
    std::vector<ca::u8> nop(65535, 0x00);   // NOP 单字节指令

    ClassFileReader accept;
    auto            cf = accept.read(buildClassWithCode(65535, nop, 0, {}));
    ASSERT_EQ(cf.getMethods().size(), 1u);
    const CodeAttribute* code = findCodeAttribute(cf.getMethods()[0]);
    ASSERT_NE(code, nullptr);
    EXPECT_EQ(code->instructions().size(), 65535u);

    ClassFileReader lenient;
    auto            rejected = lenient.read(buildClassWithCode(65536, {}, 0, {}));
    EXPECT_EQ(findCodeAttribute(rejected.getMethods()[0]), nullptr);

    ClassFileReader strict;
    strict.setDropEofAttributes(false);
    EXPECT_THROW(strict.read(buildClassWithCode(65536, {}, 0, {})), InvalidClassException);
}

// ---- 异常表条目校验（上游 805f636a + 31111e04）----

// handler_pc == code_length：handler 必须严格小于 code_length，旧版放行。
TEST(ClassFileReaderTest, ExceptionEntryWithHandlerAtCodeLengthIsDropped)
{
    std::vector<ca::u8> code(10, 0x00);
    std::vector<ca::u8> ex;
    appendExceptionEntry(ex, 0, 5, 10, 0);   // handler_pc == code_length

    ClassFileReader r;
    auto            cf   = r.read(buildClassWithCode(10, code, 1, ex));
    const CodeAttribute* codeAttr = findCodeAttribute(cf.getMethods()[0]);
    ASSERT_NE(codeAttr, nullptr);
    EXPECT_TRUE(codeAttr->exceptionTable().empty());
}

// end_pc < start_pc：逆序区间同样丢弃（旧版只丢 start_pc == end_pc）。
TEST(ClassFileReaderTest, ReversedExceptionRangeIsDropped)
{
    std::vector<ca::u8> code(10, 0x00);
    std::vector<ca::u8> ex;
    appendExceptionEntry(ex, 5, 3, 8, 0);   // end_pc < start_pc

    ClassFileReader r;
    auto            cf   = r.read(buildClassWithCode(10, code, 1, ex));
    const CodeAttribute* codeAttr = findCodeAttribute(cf.getMethods()[0]);
    ASSERT_NE(codeAttr, nullptr);
    EXPECT_TRUE(codeAttr->exceptionTable().empty());
}

// catch_type 非 0 但解析不到 CpClass（指向 Utf8 #6）：整个条目丢弃，
// 旧版把 nullptr（catch-all 语义）误入表。
TEST(ClassFileReaderTest, ExceptionEntryWithBadCatchTypeIsDropped)
{
    std::vector<ca::u8> code(10, 0x00);
    std::vector<ca::u8> ex;
    appendExceptionEntry(ex, 0, 5, 8, 6);   // #6 = Utf8 "run"，不是 CLASS

    ClassFileReader r;
    auto            cf   = r.read(buildClassWithCode(10, code, 1, ex));
    const CodeAttribute* codeAttr = findCodeAttribute(cf.getMethods()[0]);
    ASSERT_NE(codeAttr, nullptr);
    EXPECT_TRUE(codeAttr->exceptionTable().empty());
}

// 合法条目照常保留：catch_type == 0 → catch-all（catchType 为 nullptr）；
// catch_type 指向 CLASS → catchType 非 nullptr。
TEST(ClassFileReaderTest, ValidExceptionEntriesAreKept)
{
    std::vector<ca::u8> code(10, 0x00);
    std::vector<ca::u8> ex;
    appendExceptionEntry(ex, 0, 5, 8, 0);   // catch-all，合法
    appendExceptionEntry(ex, 0, 5, 8, 3);   // #3 = Class，合法

    ClassFileReader r;
    auto            cf   = r.read(buildClassWithCode(10, code, 2, ex));
    const CodeAttribute* codeAttr = findCodeAttribute(cf.getMethods()[0]);
    ASSERT_NE(codeAttr, nullptr);
    ASSERT_EQ(codeAttr->exceptionTable().size(), 2u);
    EXPECT_EQ(codeAttr->exceptionTable()[0].catchType(), nullptr);
    EXPECT_NE(codeAttr->exceptionTable()[1].catchType(), nullptr);
}

// ---- record component 上的 type annotation（上游 6489374e）----

// target_type 0x13（FIELD / empty_target）在 RECORD_COMPONENT holder 中合法：
// 修复前映射到 FIELD 与 context 不匹配而抛异常，整属性被丢弃。
TEST(ClassFileReaderTest, RecordComponentTypeAnnotationIsKept)
{
    ClassFileReader r;
    auto            cf = r.read(buildRecordWithComponentTypeAnnotation(0x13));
    ASSERT_EQ(cf.attributes().size(), 1u);

    const auto* record = static_cast<const RecordAttribute*>(cf.attributes()[0]);
    ASSERT_EQ(record->components().size(), 1u);
    const auto& attrs = record->components()[0].attributes();
    ASSERT_EQ(attrs.size(), 1u);
    EXPECT_EQ(attrs[0]->name().text(), "RuntimeVisibleTypeAnnotations");

    // roundtrip：写回后重读，注解仍在。
    auto output  = ClassFileWriter().write(cf);
    auto reread  = r.read(output);
    ASSERT_EQ(reread.attributes().size(), 1u);
    const auto* record2 = static_cast<const RecordAttribute*>(reread.attributes()[0]);
    ASSERT_EQ(record2->components().size(), 1u);
    EXPECT_EQ(record2->components()[0].attributes().size(), 1u);
}

// 未知 target_type：安全丢弃该注解，不抛异常不崩溃。
TEST(ClassFileReaderTest, RecordComponentTypeAnnotationWithUnknownTargetIsDropped)
{
    ClassFileReader lenient;
    auto            cf = lenient.read(buildRecordWithComponentTypeAnnotation(0xFF));
    // Record 属性本身保留，component 上的注解属性被安全丢弃。
    ASSERT_EQ(cf.attributes().size(), 1u);
    const auto* record = static_cast<const RecordAttribute*>(cf.attributes()[0]);
    ASSERT_EQ(record->components().size(), 1u);
    EXPECT_TRUE(record->components()[0].attributes().empty());

    // 宽容语义下重复读取始终不抛。
    ClassFileReader r2;
    EXPECT_NO_THROW(r2.read(buildRecordWithComponentTypeAnnotation(0xFF)));
}

TEST(ClassFileReaderTest, InvalidUtf8ConstantThrowsInvalidClassException)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);
    appendShort(buf, 2);   // #1 only
    buf.push_back(static_cast<ca::u8>(CpTag::UTF8));
    appendShort(buf, 4);
    buf.push_back(0xF0);
    buf.push_back(0x9F);
    buf.push_back(0x98);
    buf.push_back(0x80);   // 4-byte UTF-8 is not legal classfile MUTF-8

    ClassFileReader r;
    EXPECT_THROW(r.read(buf), InvalidClassException);
}

TEST(ClassFileReaderTest, AttributeEofCanThrowWhenConfigured)
{
    auto buf = buildClassWithAttribute();
    buf.pop_back();   // truncate one byte from attribute payload

    ClassFileReader r;
    r.setDropEofAttributes(false);
    EXPECT_THROW(r.read(buf), InvalidClassException);
}

// ============== config flag 读写 ==============

TEST(ClassFileReaderTest, ConfigFlagGettersSetters)
{
    ClassFileReader r;
    EXPECT_TRUE(r.doDropForwardVersioned());
    r.setDropForwardVersioned(false);
    EXPECT_FALSE(r.doDropForwardVersioned());
    r.setDropBadContextAttributes(false);
    EXPECT_FALSE(r.doDropBadContextAttributes());
    r.setDropEofAttributes(false);
    EXPECT_FALSE(r.doDropEofAttributes());
    r.setDropDupeAnnotations(false);
    EXPECT_FALSE(r.doDropDupeAnnotations());
    r.setCheckCodeLength(false);
    EXPECT_FALSE(r.doCheckCodeLength());
}

// ============== IndexableByteStream 直接读 raw bytes ==============

TEST(IndexableByteStreamTest, SliceSlicedReadFullyReturnsContent)
{
    std::vector<ca::u8> data = {1, 2, 3, 4, 5, 6, 7, 8};
    IndexableByteStream is(data);
    is.moveToAbsolute(2);   // skip 2 bytes
    IndexableByteStream sub(is, 4);
    std::vector<ca::u8> out;
    sub.readFully(out, 4);
    EXPECT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], 3u);
    EXPECT_EQ(out[3], 6u);
}


// ============== P1 校验族：reader 侧行为（对齐上游 2.6.7）==============

// ---- 上游 baad3a72：单例属性去重 ----

TEST(ClassFileReaderTest, DeduplicatesDuplicateSingletonAttributesOnRead)
{
    // 语料：class 层 2 个 SourceFile，<init> 上 2 个 Code（均同型重复）。
    auto bytes = test::getResource("obfuscated/misc/DuplicateAttributeFixture.class");
    ASSERT_FALSE(bytes.empty());

    ClassFileReader r;
    auto            cf = r.read(bytes);

    // SourceFile 只保留首个。
    int sourceFileCount = 0;
    for (const Attribute* a : cf.attributes()) {
        if (a->name().text() == "SourceFile") ++sourceFileCount;
    }
    EXPECT_EQ(sourceFileCount, 1);

    // 每个方法至多一个 Code。
    for (const Method* m : cf.getMethods()) {
        int codeCount = 0;
        for (const Attribute* a : m->attributes()) {
            if (a->name().text() == "Code") ++codeCount;
        }
        EXPECT_LE(codeCount, 1) << "method " << m->name()->text();
    }
}

TEST(ClassFileReaderTest, IsSingletonAttributeList)
{
    EXPECT_TRUE(AttributeReader::isSingletonAttribute("Code"));
    EXPECT_TRUE(AttributeReader::isSingletonAttribute("ConstantValue"));
    EXPECT_TRUE(AttributeReader::isSingletonAttribute("BootstrapMethods"));
    EXPECT_TRUE(AttributeReader::isSingletonAttribute("SourceFile"));
    EXPECT_TRUE(AttributeReader::isSingletonAttribute("StackMapTable"));
    // 非单例：可重复。
    EXPECT_FALSE(AttributeReader::isSingletonAttribute("LineNumberTable"));
    EXPECT_FALSE(AttributeReader::isSingletonAttribute("LocalVariableTable"));
    EXPECT_TRUE(AttributeReader::isSingletonAttribute("RuntimeVisibleTypeAnnotations"));
    EXPECT_TRUE(AttributeReader::isSingletonAttribute("RuntimeInvisibleTypeAnnotations"));
    EXPECT_FALSE(AttributeReader::isSingletonAttribute("Custom"));
}

// ---- 上游 7cc58e40：BSM 畸形条目跳过而非整属性丢弃 + condy 索引重映射 ----

// BSM 表 2 条目：entry0 携带越界 handle/参数（畸形，读取时跳过）；entry1 合法。
// condy 常量原始 bsmIndex=1，读取后应重映射为 0。
static std::vector<ca::u8> buildClassWithPartiallyMalformedBootstrap()
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);

    // Pool:
    //   #1  Utf8 "Test"
    //   #2  Utf8 "java/lang/Object"
    //   #3  Class -> #1 (this)
    //   #4  Class -> #2 (super)
    //   #5  Utf8 "BootstrapMethods"
    //   #6  Utf8 "Ljava/lang/String;"
    //   #7  Utf8 "x"
    //   #8  NameType(#7, #6)
    //   #9  FieldRef(#3, #8)
    //   #10 MethodHandle(GET_STATIC, #9)
    //   #11 Dynamic(bsm=1, #8)
    //   #12 Utf8 "Code"
    //   #13 Utf8 "run"
    //   #14 Utf8 "()V"
    appendShort(buf, 15);
    appendUtf8Entry(buf, "Test");
    appendUtf8Entry(buf, "java/lang/Object");
    appendClassEntry(buf, 1);
    appendClassEntry(buf, 2);
    appendUtf8Entry(buf, "BootstrapMethods");
    appendUtf8Entry(buf, "Ljava/lang/String;");
    appendUtf8Entry(buf, "x");
    appendNameTypeEntry(buf, 7, 6);
    buf.push_back(static_cast<ca::u8>(CpTag::FIELD_REF));
    appendShort(buf, 3);
    appendShort(buf, 8);
    buf.push_back(static_cast<ca::u8>(CpTag::METHOD_HANDLE));
    buf.push_back(2);   // GET_STATIC
    appendShort(buf, 9);
    buf.push_back(static_cast<ca::u8>(CpTag::DYNAMIC));
    appendShort(buf, 1);   // bsmIndex -> entry1
    appendShort(buf, 8);
    appendUtf8Entry(buf, "Code");
    appendUtf8Entry(buf, "run");
    appendUtf8Entry(buf, "()V");

    appendShort(buf, 0x0021);
    appendShort(buf, 3);   // this
    appendShort(buf, 4);   // super
    appendShort(buf, 0);   // interfaces
    appendShort(buf, 0);   // fields

    // 1 method: aload_0, ldc #11（引用 condy）
    appendShort(buf, 1);
    appendShort(buf, 0x0001);
    appendShort(buf, 13);
    appendShort(buf, 14);
    appendShort(buf, 1);   // 1 attr: Code
    std::vector<ca::u8> code = {0x2A, 0x12, 11};
    ca::u32             body = 2 + 2 + 4 + static_cast<ca::u32>(code.size()) + 2 + 2;
    appendShort(buf, 12);   // #12 = "Code"
    appendInt(buf, body);
    appendShort(buf, 1);   // max_stack
    appendShort(buf, 1);   // max_locals
    appendInt(buf, static_cast<ca::u32>(code.size()));
    for (ca::u8 b : code) buf.push_back(b);
    appendShort(buf, 0);   // exceptions
    appendShort(buf, 0);   // code attrs

    // 1 class attr: BootstrapMethods
    // entry0: handle=99（越界 → 畸形）；entry1: handle=#10（合法）
    appendShort(buf, 1);   // 1 class attr
    appendShort(buf, 5);   // name idx
    appendInt(buf, 10);
    appendShort(buf, 2);   // count
    appendShort(buf, 99);  // entry0 handle
    appendShort(buf, 0);   // entry0 args
    appendShort(buf, 10);  // entry1 handle
    appendShort(buf, 0);   // entry1 args
    return buf;
}

TEST(ClassFileReaderTest, SkipsMalformedBootstrapEntryAndRemapsCondyIndex)
{
    ClassFileReader r;
    auto            cf = r.read(buildClassWithPartiallyMalformedBootstrap());

    // 旧实现：任一条目畸形 → 整个 BSM 属性丢弃（过度剥离）。新行为：跳过
    // 畸形条目，保留属性，并重映射 condy 的 bsm 索引。
    const Attribute* bsmAttr = nullptr;
    for (const Attribute* a : cf.attributes()) {
        if (a->name().text() == "BootstrapMethods") bsmAttr = a;
    }
    ASSERT_NE(bsmAttr, nullptr);
    const auto* bsm = static_cast<const BootstrapMethodsAttribute*>(bsmAttr);
    ASSERT_EQ(bsm->bootstrapMethods().size(), 1u);
    EXPECT_EQ(bsm->bootstrapMethods()[0].bsmMethodRef()->kind(), CpMethodHandle::REF_GET_STATIC);

    // condy 原始 bsmIndex=1 → 重映射为 0。
    const ConstDynamic* condy = nullptr;
    cf.getPool().iterator().forEach([&](int, const CpEntry* e) {
        if (e->isConstDynamic()) condy = static_cast<const ConstDynamic*>(e);
    });
    ASSERT_NE(condy, nullptr);
    EXPECT_EQ(condy->bsmIndex(), 0);
}

TEST(ClassFileReaderTest, DropsBootstrapAttributeWhenAllEntriesMalformed)
{
    // 与上例同一 class，但把 entry1 的 handle 索引也改成越界：全部条目畸形 →
    // 属性整体丢弃（与旧实现行为一致）。
    auto buf = buildClassWithPartiallyMalformedBootstrap();
    // BSM 内容里 entry1 的 handle 是最后一个 short，改写为 99。
    size_t tail      = buf.size();
    buf[tail - 4]    = 0;
    buf[tail - 3]    = 99;

    ClassFileReader r;
    auto            cf = r.read(buf);
    for (const Attribute* a : cf.attributes()) {
        EXPECT_NE(a->name().text(), "BootstrapMethods");
    }
}

// ---- 上游 f19d7129：注解枚举元素名校验 ----

// RuntimeVisibleAnnotations：LA; 中 value 元素为 enum，typename/constname 可控。
static std::vector<ca::u8> buildClassWithEnumElement(ca::u16 typeNameIdx, ca::u16 constNameIdx)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);

    // Pool:
    //   #1 Utf8 "Test"          #2 Utf8 "java/lang/Object"
    //   #3 Class -> #1          #4 Class -> #2
    //   #5 Utf8 "RuntimeVisibleAnnotations"
    //   #6 Utf8 "LA;"
    //   #7 Utf8 "value"
    //   #8 Utf8 "Ljava/lang/annotation/Retention;"
    //   #9 Utf8 "RUNTIME"（合法非限定名）
    //   #10 Utf8 "RUN/TIME"（非法：含 '/'）
    appendShort(buf, 11);
    appendUtf8Entry(buf, "Test");
    appendUtf8Entry(buf, "java/lang/Object");
    appendClassEntry(buf, 1);
    appendClassEntry(buf, 2);
    appendUtf8Entry(buf, "RuntimeVisibleAnnotations");
    appendUtf8Entry(buf, "LA;");
    appendUtf8Entry(buf, "value");
    appendUtf8Entry(buf, "Ljava/lang/annotation/Retention;");
    appendUtf8Entry(buf, "RUNTIME");
    appendUtf8Entry(buf, "RUN/TIME");

    appendShort(buf, 0x0021);
    appendShort(buf, 3);
    appendShort(buf, 4);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 1);   // 1 class attr

    appendShort(buf, 5);
    // content: u2 num_annotations=1, u2 type=#6, u2 pairs=1,
    //          u2 name=#7, u1 tag='e', u2 typename, u2 constname
    //          → 2+2+2+2+1+2+2 = 13
    appendInt(buf, 13);
    appendShort(buf, 1);
    appendShort(buf, 6);
    appendShort(buf, 1);
    appendShort(buf, 7);
    buf.push_back(static_cast<ca::u8>('e'));
    appendShort(buf, typeNameIdx);
    appendShort(buf, constNameIdx);
    return buf;
}

TEST(ClassFileReaderTest, EnumElementWithInvalidConstNameIsRejected)
{
    ClassFileReader r;
    auto            cf = r.read(buildClassWithEnumElement(8, 10));   // "RUN/TIME" 非法
    EXPECT_TRUE(cf.attributes().empty());

    auto ok = r.read(buildClassWithEnumElement(8, 9));   // "RUNTIME" 合法
    ASSERT_EQ(ok.attributes().size(), 1u);
    EXPECT_EQ(ok.attributes()[0]->name().text(), "RuntimeVisibleAnnotations");
}

TEST(ClassFileReaderTest, EnumElementWithNonObjectTypeNameIsRejected)
{
    // typename 指向 #7（"value"，非 L...; 形态）：必须为对象描述符 → 拒绝。
    ClassFileReader r;
    auto            cf = r.read(buildClassWithEnumElement(7, 9));
    EXPECT_TRUE(cf.attributes().empty());
}

// ---- 上游 66d31cf8：primitive 元素值 tag 与常量种类匹配 ----

// RuntimeVisibleAnnotations：LA; 中 value 元素为 primitive，index 可控。
static std::vector<ca::u8> buildClassWithPrimitiveElement(char tag, ca::u16 index)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);

    // Pool: #1..#4 类头；#5 attr 名；#6 "LA;"；#7 "value"；#8 INTEGER(1)；
    //       #9 STRING -> #10；#10 Utf8 "s"
    appendShort(buf, 11);
    appendUtf8Entry(buf, "Test");
    appendUtf8Entry(buf, "java/lang/Object");
    appendClassEntry(buf, 1);
    appendClassEntry(buf, 2);
    appendUtf8Entry(buf, "RuntimeVisibleAnnotations");
    appendUtf8Entry(buf, "LA;");
    appendUtf8Entry(buf, "value");
    appendIntegerEntry(buf, 1);
    buf.push_back(static_cast<ca::u8>(CpTag::STRING));
    appendShort(buf, 10);
    appendUtf8Entry(buf, "s");

    appendShort(buf, 0x0021);
    appendShort(buf, 3);
    appendShort(buf, 4);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 1);

    appendShort(buf, 5);
    // content: u2 num_annotations=1, u2 type=#6, u2 pairs=1,
    //          u2 name=#7, u1 tag, u2 index → 2+2+2+2+1+2 = 11
    appendInt(buf, 11);
    appendShort(buf, 1);
    appendShort(buf, 6);
    appendShort(buf, 1);
    appendShort(buf, 7);
    buf.push_back(static_cast<ca::u8>(tag));
    appendShort(buf, index);
    return buf;
}

TEST(ClassFileReaderTest, PrimitiveElementTagMismatchIsRejected)
{
    ClassFileReader r;
    // 'I' 期望 INTEGER(#8)，给 STRING(#9)：种类不匹配 → 属性拒绝。
    auto bad = r.read(buildClassWithPrimitiveElement('I', 9));
    EXPECT_TRUE(bad.attributes().empty());

    // 'I' + INTEGER(#8)：匹配 → 保留。
    auto ok = r.read(buildClassWithPrimitiveElement('I', 8));
    ASSERT_EQ(ok.attributes().size(), 1u);
}
