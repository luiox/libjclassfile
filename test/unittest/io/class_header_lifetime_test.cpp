// issue #876 回归钉：类头三组 CP 引用（thisClass/superClass/interfaces）的
// 生命周期。
//
// 根因：read() 收尾的 placeholder 清理（class_file_reader.cpp）会销毁"含
// placeholder 引用"的池条目，但类头引用不经属性粒度的 isOrContainsPlaceholder
// 丢弃检查。敌意样本（this_class 指向 name_index 越界的 CLASS 条目）在旧代码
// 下让 ClassFile 头部持有悬垂指针，ClassFileWriter::write 解引用已释放对象，
// release 构建输出垃圾字节（version/this/super 偏移，布局敏感、ASan -O0 不
// 复现）。修复后：仍被类头引用的条目保留，无引用者照常清理。
//
// 本文件所有用例在 debug/release 下都必须通过；write → re-read 的头部一致性
// 是布局漂移 tripwire。

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "libca/core/datatype.hpp"
#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/transform/illegal_stripping_transformer.hpp"
#include "test_helper.hpp"

namespace {

using namespace jclassfile;

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
    buf.push_back(static_cast<ca::u8>(model::CpTag::UTF8));
    appendShort(buf, static_cast<ca::u16>(s.size()));
    for (char c : s) buf.push_back(static_cast<ca::u8>(c));
}

void appendClassEntry(std::vector<ca::u8>& buf, ca::u16 utf8Index)
{
    buf.push_back(static_cast<ca::u8>(model::CpTag::CLASS));
    appendShort(buf, utf8Index);
}

// this_class 指向 name_index 越界的 CLASS 条目（resolve 后 name 保持
// placeholder），super_class 正常。池布局：
//   #1  Class -> 0xFFFE（坏名，this）
//   #2  Utf8 "Super"
//   #3  Class -> #2（super）
TEST(ClassHeaderLifetimeTest, ThisClassPlaceholderEntrySurvivesCleanup)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);
    appendShort(buf, 4);
    appendClassEntry(buf, 0xFFFE);
    appendUtf8Entry(buf, "Super");
    appendClassEntry(buf, 2);
    appendShort(buf, 0x0021);
    appendShort(buf, 1);   // this = #1（坏名条目）
    appendShort(buf, 3);   // super = #3
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 0);

    io::ClassFileReader r;
    auto                cf = r.read(buf);
    // 条目被类头引用，必须保留（旧代码在此处销毁 → thisClass 悬垂）
    ASSERT_NE(cf.getThisClass(), nullptr);
    EXPECT_TRUE(cf.getCp(1) == static_cast<const model::CpEntry*>(cf.getThisClass()));

    transform::IllegalStrippingTransformer(cf).transform();
    auto output = io::ClassFileWriter().write(cf);

    auto cleaned = r.read(output);
    ASSERT_NE(cleaned.getThisClass(), nullptr);
    EXPECT_EQ(cleaned.getVersionMajor(), 52);
}

// interfaces 里的坏名 CLASS 条目同样被类头引用，必须保留。
TEST(ClassHeaderLifetimeTest, InterfacePlaceholderEntrySurvivesCleanup)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);
    appendShort(buf, 6);
    appendUtf8Entry(buf, "Super");
    appendClassEntry(buf, 1);    // #2 super，正常
    appendClassEntry(buf, 0xFFFE);   // #3 坏名，interface
    appendUtf8Entry(buf, "Unused");
    appendClassEntry(buf, 0xFFFE);   // #5 无引用坏名条目（应被清理）
    appendShort(buf, 0x0021);
    appendShort(buf, 2);
    appendShort(buf, 0);
    appendShort(buf, 1);   // 1 interface
    appendShort(buf, 3);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 0);
    appendShort(buf, 0);

    io::ClassFileReader r;
    auto                cf = r.read(buf);
    ASSERT_EQ(cf.getInterfaceClasses().size(), 1u);
    ASSERT_NE(cf.getInterfaceClasses()[0], nullptr);
    // 被接口引用的 #3 保留
    EXPECT_TRUE(cf.getCp(3) ==
                static_cast<const model::CpEntry*>(cf.getInterfaceClasses()[0]));
    // 坏名 Class 条目（含 placeholder 间接引用）不参与清理：清理只删
    // "自身引用含 placeholder"的条目（未知 tag 占位等），CpClass 不在
    // cross-referencing 集合内，与上游 CAFED00D 行为一致
    EXPECT_NE(cf.getCp(5), nullptr);

    auto output = io::ClassFileWriter().write(cf);
    auto cleaned = r.read(output);
    ASSERT_EQ(cleaned.getInterfaceClasses().size(), 1u);
    ASSERT_NE(cleaned.getInterfaceClasses()[0], nullptr);
}

// issue #876 探针固化：proguard 全样本 read → transform → write → re-read，
// 头部身份（version/this/super）必须逐字节稳定。
TEST(ClassHeaderLifetimeTest, WritePreservesHeaderIdentityAfterTransform)
{
    auto root = test::getResourcePath("obfuscated/proguard");
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "proguard samples not found: " << root;
    }

    int filesChecked = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        const auto rel = std::filesystem::relative(entry.path(), root);
        auto       cf  = io::ClassFileReader().read(
            test::getResource("obfuscated/proguard/" + rel.generic_string()));

        const int  versionMajor = cf.getVersionMajor();
        const int  versionMinor = cf.getVersionMinor();
        const int  access       = cf.getAccess();
        const auto thisName     = cf.getThisClass() ? cf.getThisClass()->name()->text() : "";
        const auto superName    = cf.getSuperClass() ? cf.getSuperClass()->name()->text() : "";

        transform::IllegalStrippingTransformer(cf).transform();
        auto       output  = io::ClassFileWriter().write(cf);
        const auto cleaned = io::ClassFileReader().read(output);

        EXPECT_EQ(cleaned.getVersionMajor(), versionMajor) << rel.generic_string();
        EXPECT_EQ(cleaned.getVersionMinor(), versionMinor) << rel.generic_string();
        EXPECT_EQ(cleaned.getAccess(), access) << rel.generic_string();
        ASSERT_NE(cleaned.getThisClass(), nullptr) << rel.generic_string();
        EXPECT_EQ(cleaned.getThisClass()->name()->text(), thisName) << rel.generic_string();
        if (cf.getSuperClass() == nullptr) {
            EXPECT_EQ(cleaned.getSuperClass(), nullptr) << rel.generic_string();
        }
        else {
            ASSERT_NE(cleaned.getSuperClass(), nullptr) << rel.generic_string();
            EXPECT_EQ(cleaned.getSuperClass()->name()->text(), superName) << rel.generic_string();
        }
        ++filesChecked;
    }
    EXPECT_GT(filesChecked, 0);
}

}   // namespace
