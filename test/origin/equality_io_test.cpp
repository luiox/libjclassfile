// CAFED00D 1:1 mirror: EqualityIOTest.java
//
// [UNALIGNED] Java version has two test sources:
//   1. supplyResourceClasses() - reads from "src/test/resources/samples/normal"
//   2. supplySelfClasses() - reads from "target/classes" (self-compiled classes)
// We only test resource classes since self-compiled classes depend on build output.

#include <gtest/gtest.h>

#include <filesystem>

#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/invalid_class_exception.hpp"
#include "jclassfile/model/class_file.hpp"
#include "test_helper.hpp"
#include "jclassfile/transform/illegal_stripping_transformer.hpp"

namespace {

void testSubdir(const std::string& subdir)
{
    auto root = test::getResourcePath("normal/" + subdir);
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << subdir << " not found";
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource("normal/" + subdir + "/" + rel.generic_string());
        if (bytes.empty()) continue;

        try {
            auto cf = jclassfile::io::ClassFileReader().read(bytes);
            jclassfile::transform::IllegalStrippingTransformer(cf).transform();
            auto out = jclassfile::io::ClassFileWriter().write(cf);
            EXPECT_EQ(bytes.size(), out.size()) << entry.path();
            EXPECT_EQ(bytes, out) << entry.path();
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

TEST(EqualityIOTest, NormalJavac)
{
    testSubdir("javac");
}
TEST(EqualityIOTest, NormalAnnos)
{
    testSubdir("annos");
}
TEST(EqualityIOTest, NormalMisc)
{
    testSubdir("misc");
}
TEST(EqualityIOTest, NormalModules)
{
    testSubdir("modules");
}
