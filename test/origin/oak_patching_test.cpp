// CAFED00D 1:1 mirror: OakPatchingTest.java
//
// [UNALIGNED] Java version includes ASM verification:
//   1. assertDoesNotThrow(() -> { ASM reads modified; }) - verify ASM works after patching
//   2. Comments about "sample1" crashing ASM and "sample2" yielding wrong code
// We skip ASM verification since it depends on ASM library.

#include <gtest/gtest.h>

#include <filesystem>

#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/invalid_class_exception.hpp"
#include "jclassfile/model/class_file.hpp"
#include "test_helper.hpp"

TEST(OakPatchingTest, OakSamples)
{
    auto root = test::getResourcePath("oak");
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "oak not found";
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource("oak/" + rel.generic_string());
        if (bytes.empty()) continue;

        try {
            auto cf       = jclassfile::io::ClassFileReader().read(bytes);
            auto modified = jclassfile::io::ClassFileWriter().write(cf);
            (void)modified;
        }
        catch (const jclassfile::InvalidClassException& e) {
            FAIL() << rel.generic_string() << ": " << e.what();
        }
        catch (const std::exception& e) {
            FAIL() << rel.generic_string() << ": " << e.what();
        }
    }
}
