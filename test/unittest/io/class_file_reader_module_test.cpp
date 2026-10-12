// 上游 cffb89ff：带 ACC_MODULE 的 class 声明了 superclass/interface/field/method
// 等成员时按 JVMS 语义降级为普通类——清除 ACC_MODULE、丢弃模块元数据属性、
// 清理池中不再可引用的 CpModule/CpPackage 条目；结构纯净的 module-info 保持原样。
// 全部用例以最小手工字节构造，不依赖二进制语料。

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "libca/core/datatype.hpp"

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

void appendModuleEntry(std::vector<ca::u8>& buf, ca::u16 utf8Index)
{
    buf.push_back(static_cast<ca::u8>(CpTag::MODULE));
    appendShort(buf, utf8Index);
}

void appendPackageEntry(std::vector<ca::u8>& buf, ca::u16 utf8Index)
{
    buf.push_back(static_cast<ca::u8>(CpTag::PACKAGE));
    appendShort(buf, utf8Index);
}

// 池布局（major = 53 / JAVA9）：
//   #1  Utf8 "module-info"
//   #2  Class -> #1          （this_class）
//   #3  Utf8 "TestModule"
//   #4  Module -> #3
//   #5  Utf8 "Module"        （属性名）
//   #6  Utf8 "java/lang/Object"
//   #7  Class -> #6          （super 候选）
//   #8  Utf8 "SomeInterface"
//   #9  Class -> #8          （interface 候选）
//   #10 Utf8 "member"
//   #11 Utf8 "I"
//   [#12 Utf8 "SomePackage"
//   #13 Package -> #12]      （仅 withPackageCp）
// numConstants = withPackageCp ? 14 : 12
struct ModuleClassSpec
{
    ca::u16 access         = 0x8000;   // ACC_MODULE
    ca::u16 superIndex     = 0;        // 0 = 无 superclass
    int     interfaceCount = 0;
    int     fieldCount     = 0;
    int     methodCount    = 0;
    bool    withModuleAttr = false;
    bool    withPackageCp  = false;
};

std::vector<ca::u8> buildModuleClass(const ModuleClassSpec& spec)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 53);   // JAVA9：Module 属性合法版本

    appendShort(buf, spec.withPackageCp ? 14 : 12);
    appendUtf8Entry(buf, "module-info");          // #1
    appendClassEntry(buf, 1);                     // #2
    appendUtf8Entry(buf, "TestModule");           // #3
    appendModuleEntry(buf, 3);                    // #4
    appendUtf8Entry(buf, "Module");               // #5
    appendUtf8Entry(buf, "java/lang/Object");     // #6
    appendClassEntry(buf, 6);                     // #7
    appendUtf8Entry(buf, "SomeInterface");        // #8
    appendClassEntry(buf, 8);                     // #9
    appendUtf8Entry(buf, "member");               // #10
    appendUtf8Entry(buf, "I");                    // #11
    if (spec.withPackageCp) {
        appendUtf8Entry(buf, "SomePackage");      // #12
        appendPackageEntry(buf, 12);              // #13
    }

    appendShort(buf, spec.access);
    appendShort(buf, 2);                  // this = #2
    appendShort(buf, spec.superIndex);    // super（module 合法值为 0）
    appendShort(buf, static_cast<ca::u16>(spec.interfaceCount));
    for (int i = 0; i < spec.interfaceCount; i++) {
        appendShort(buf, 9);              // interface = #9
    }

    appendShort(buf, static_cast<ca::u16>(spec.fieldCount));
    for (int i = 0; i < spec.fieldCount; i++) {
        appendShort(buf, 0x0002);         // ACC_PRIVATE
        appendShort(buf, 10);             // name = #10
        appendShort(buf, 11);             // desc = #11
        appendShort(buf, 0);
    }

    appendShort(buf, static_cast<ca::u16>(spec.methodCount));
    for (int i = 0; i < spec.methodCount; i++) {
        appendShort(buf, 0x0001);         // ACC_PUBLIC
        appendShort(buf, 10);             // name = #10
        appendShort(buf, 11);             // desc = #11
        appendShort(buf, 0);
    }

    appendShort(buf, spec.withModuleAttr ? 1 : 0);
    if (spec.withModuleAttr) {
        // Module 属性：name=#5，长度 16
        // body = u2 module_index + u2 flags + u2 version_index
        //      + u2 requires_count + u2 exports_count + u2 opens_count
        //      + u2 uses_count + u2 provides_count
        appendShort(buf, 5);
        appendInt(buf, 16);
        appendShort(buf, 4);              // module_index = #4
        appendShort(buf, 0);              // flags
        appendShort(buf, 0);              // version_index
        appendShort(buf, 0);              // requires_count
        appendShort(buf, 0);              // exports_count
        appendShort(buf, 0);              // opens_count
        appendShort(buf, 0);              // uses_count
        appendShort(buf, 0);              // provides_count
    }
    return buf;
}

bool hasModulePoolEntry(const ClassFile& clazz, int index)
{
    const CpEntry* entry = clazz.getPool().get(index);
    return entry != nullptr && entry->tag() == CpTag::MODULE;
}

bool hasPackagePoolEntry(const ClassFile& clazz, int index)
{
    const CpEntry* entry = clazz.getPool().get(index);
    return entry != nullptr && entry->tag() == CpTag::PACKAGE;
}

bool hasAttributeNamed(const ClassFile& clazz, const char* name)
{
    for (const Attribute* attr : clazz.attributes()) {
        if (attr->name().text() == name) return true;
    }
    return false;
}

}   // namespace

// ---------- 结构纯净的 module-info：不降级 ----------

TEST(ClassFileReaderModuleTest, PureModuleClassKeepsModuleAccessAndMetadata)
{
    ModuleClassSpec spec;
    spec.withModuleAttr = true;
    ClassFileReader     reader;
    const ClassFile     clazz = reader.read(buildModuleClass(spec));

    EXPECT_EQ(clazz.getAccess(), 0x8000);
    EXPECT_TRUE(hasAttributeNamed(clazz, "Module"));
    EXPECT_TRUE(hasModulePoolEntry(clazz, 4));
}

TEST(ClassFileReaderModuleTest, PureModuleClassKeepsCpPackage)
{
    ModuleClassSpec spec;
    spec.withPackageCp = true;
    ClassFileReader    reader;
    const ClassFile    clazz = reader.read(buildModuleClass(spec));

    EXPECT_EQ(clazz.getAccess(), 0x8000);
    EXPECT_TRUE(hasPackagePoolEntry(clazz, 13));
}

// ---------- 声明了成员的 module class：降级为普通类 ----------

TEST(ClassFileReaderModuleTest, ModuleClassWithSuperClassIsDowngraded)
{
    ModuleClassSpec spec;
    spec.superIndex     = 7;
    spec.withModuleAttr = true;
    ClassFileReader     reader;
    const ClassFile     clazz = reader.read(buildModuleClass(spec));

    // ACC_MODULE 被清除，模块元数据属性与 CpModule 一并丢弃。
    EXPECT_EQ(clazz.getAccess(), 0);
    EXPECT_FALSE(hasAttributeNamed(clazz, "Module"));
    EXPECT_FALSE(hasModulePoolEntry(clazz, 4));
    // 结构成员本身保留。
    EXPECT_NE(clazz.getSuperClass(), nullptr);
    EXPECT_EQ(clazz.attributes().size(), 0u);
}

TEST(ClassFileReaderModuleTest, ModuleClassWithInterfaceIsDowngraded)
{
    ModuleClassSpec spec;
    spec.interfaceCount = 1;
    ClassFileReader     reader;
    const ClassFile     clazz = reader.read(buildModuleClass(spec));

    EXPECT_EQ(clazz.getAccess(), 0);
    EXPECT_FALSE(hasModulePoolEntry(clazz, 4));
    EXPECT_EQ(clazz.getInterfaceClasses().size(), 1u);
}

TEST(ClassFileReaderModuleTest, ModuleClassWithFieldIsDowngraded)
{
    ModuleClassSpec spec;
    spec.fieldCount    = 1;
    spec.withPackageCp = true;
    ClassFileReader    reader;
    const ClassFile    clazz = reader.read(buildModuleClass(spec));

    EXPECT_EQ(clazz.getAccess(), 0);
    EXPECT_EQ(clazz.getFields().size(), 1u);
    EXPECT_FALSE(hasModulePoolEntry(clazz, 4));
    EXPECT_FALSE(hasPackagePoolEntry(clazz, 13));
}

TEST(ClassFileReaderModuleTest, ModuleClassWithMethodIsDowngraded)
{
    ModuleClassSpec spec;
    spec.methodCount = 1;
    ClassFileReader  reader;
    const ClassFile  clazz = reader.read(buildModuleClass(spec));

    EXPECT_EQ(clazz.getAccess(), 0);
    EXPECT_EQ(clazz.getMethods().size(), 1u);
    EXPECT_FALSE(hasModulePoolEntry(clazz, 4));
}

TEST(ClassFileReaderModuleTest, DowngradedModuleClassDropsCpPackageToo)
{
    ModuleClassSpec spec;
    spec.superIndex    = 7;
    spec.withPackageCp = true;
    ClassFileReader    reader;
    const ClassFile    clazz = reader.read(buildModuleClass(spec));

    EXPECT_EQ(clazz.getAccess(), 0);
    EXPECT_FALSE(hasPackagePoolEntry(clazz, 13));
}

// ---------- 普通 class：moduleAccess 门控不误伤 ----------

TEST(ClassFileReaderModuleTest, PlainClassWithMembersIsNotDowngraded)
{
    ModuleClassSpec spec;
    spec.access     = 0x0021;   // ACC_PUBLIC | ACC_SUPER
    spec.superIndex = 7;
    spec.fieldCount = 1;
    ClassFileReader reader;
    const ClassFile clazz = reader.read(buildModuleClass(spec));

    EXPECT_EQ(clazz.getAccess(), 0x0021);
    EXPECT_EQ(clazz.getFields().size(), 1u);
}
