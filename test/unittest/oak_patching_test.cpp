// 阶段 7：Oak 样本读取不崩溃测试
// 对齐 CAFED00D OakPatchingTest

#include <gtest/gtest.h>

#include <filesystem>

#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/invalid_class_exception.hpp"
#include "test_helper.hpp"

namespace {

void testNoCrash(const std::string& relPath)
{
    auto root = test::getResourcePath(relPath);
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "Sample dir not found: " << root;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource(relPath + "/" + rel.generic_string());
        if (bytes.empty()) continue;

        // CAFED00D OakPatchingTest: read + write, verify no crash
        jclassfile::io::ClassFileReader reader;
        try {
            auto                            cf = reader.read(bytes);
            jclassfile::io::ClassFileWriter writer;
            auto                            rewritten = writer.write(cf);
            (void)rewritten;
        }
        catch (const jclassfile::InvalidClassException& e) {
            FAIL() << rel.generic_string() << ": " << e.what();
        }
        catch (const std::exception& e) {
            FAIL() << rel.generic_string() << ": " << e.what();
        }
    }
}

}   // namespace

TEST(OakPatchingTest, OakSamples)
{
    testNoCrash("oak");
}
