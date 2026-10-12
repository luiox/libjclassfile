// CAFED00D 1:1 mirror: CrasherPatchingTest.java
//
// [UNALIGNED] Java version includes ASM verification:
//   1. assertThrows(Throwable.class, () -> { ASM reads code; }) - verify ASM fails before patching
//   2. assertDoesNotThrow(() -> { ASM reads modified; }) - verify ASM works after patching
//   3. noverify_code set for samples with unverifiable bytecode (sample35/36/48.class)
//   4. TestClassWriter with getCommonSuperClass returning "java/lang/Object"
// We skip ASM verification since it depends on ASM library.

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#include "jclassfile/io/class_builder.hpp"
#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/io/fallback_instruction_reader.hpp"
#include "jclassfile/invalid_class_exception.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "test_helper.hpp"
#include "jclassfile/transform/illegal_rewriting_instructions_reader.hpp"
#include "jclassfile/transform/illegal_stripping_transformer.hpp"

namespace {

class ReservedOpcodeRewritingReader : public jclassfile::io::ClassFileReader
{
public:
    std::unique_ptr<jclassfile::io::FallbackInstructionReader> getFallbackInstructionReader(
        jclassfile::io::ClassBuilder& builder) override
    {
        return std::make_unique<jclassfile::transform::IllegalRewritingInstructionsReader>(
            builder.getPool(), builder.getVersionMajor());
    }
};

}   // namespace

TEST(CrasherPatchingTest, CrasherAsm)
{
    auto root = test::getResourcePath("obfuscated/crasher-asm");
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "samples not found";
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource("obfuscated/crasher-asm/" + rel.generic_string());
        if (bytes.empty()) continue;

        ReservedOpcodeRewritingReader reader;
        try {
            auto cf = reader.read(bytes);
            jclassfile::transform::IllegalStrippingTransformer(cf).transform();
            auto rewritten = jclassfile::io::ClassFileWriter().write(cf);
            (void)rewritten;
        }
        catch (const jclassfile::InvalidClassException& e) {
            FAIL() << "InvalidClassException: " << rel.generic_string() << ": " << e.what();
        }
        catch (const std::exception& e) {
            FAIL() << "Error: " << rel.generic_string() << ": " << e.what();
        }
    }
}
