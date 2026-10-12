// CAFED00D 1:1 mirror: RegressionTests.java

#include <gtest/gtest.h>

#include <filesystem>

#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/invalid_class_exception.hpp"
#include "jclassfile/model/class_file.hpp"
#include "test_helper.hpp"

TEST(RegressionTests, CrasherDude)
{
    auto root = test::getResourcePath("obfuscated/crasher-dude");
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "crasher-dude not found";
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource("obfuscated/crasher-dude/" + rel.generic_string());
        if (bytes.empty()) continue;

        try {
            jclassfile::io::ClassFileReader reader;
            auto                            cf  = reader.read(bytes);
            auto                            out = jclassfile::io::ClassFileWriter().write(cf);
            (void)out;
        }
        catch (const jclassfile::InvalidClassException& e) {
            FAIL() << rel.generic_string() << ": " << e.what();
        }
        catch (const std::exception& e) {
            FAIL() << rel.generic_string() << ": " << e.what();
        }
    }
}
