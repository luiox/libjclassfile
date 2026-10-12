// 阶段 7：全样本 read → write → 字节级对比
// 对齐 CAFED00D EqualityIOTest

#include <gtest/gtest.h>

#include <filesystem>

#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "test_helper.hpp"

namespace {

void testRoundTrip(const std::string& relPath)
{
    auto root = test::getResourcePath(relPath);
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "Sample dir not found: " << root;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel      = std::filesystem::relative(entry.path(), root);
        auto original = test::getResource(relPath + "/" + rel.generic_string());
        if (original.empty()) continue;

        // Read
        jclassfile::io::ClassFileReader reader;
        auto                            cf = reader.read(original);
        // Write
        jclassfile::io::ClassFileWriter writer;
        std::vector<ca::u8>             rewritten;
        try {
            rewritten = writer.write(cf);
        }
        catch (const std::exception& e) {
            FAIL() << "Write failed for " << entry.path() << ": " << e.what();
        }

        // Compare
        EXPECT_EQ(original.size(), rewritten.size()) << "Size mismatch: " << entry.path();
        if (original.size() == rewritten.size()) {
            EXPECT_EQ(original, rewritten) << "Byte mismatch: " << entry.path();
        }
    }
}

}   // namespace

TEST(EqualityIOTest, NormalJavac)
{
    testRoundTrip("normal/javac");
}

TEST(EqualityIOTest, NormalAnnos)
{
    testRoundTrip("normal/annos");
}

TEST(EqualityIOTest, NormalMisc)
{
    testRoundTrip("normal/misc");
}

TEST(EqualityIOTest, NormalModules)
{
    testRoundTrip("normal/modules");
}
