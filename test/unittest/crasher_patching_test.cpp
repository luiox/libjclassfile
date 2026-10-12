// 原版：ref_samples/CAFED00D/core/src/test/java/software/coley/cafedude/CrasherPatchingTest.java

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

// CAFED00D ReservedOpcodeRewritingReader: overrides getFallbackInstructionReader() to
// return an IllegalRewritingInstructionsReader that rewrites reserved HotSpot VM opcodes.
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

// CAFED00D CrasherPatchingTest: read obfuscated crasher samples using
// ReservedOpcodeRewritingReader, apply IllegalStrippingTransformer to strip invalid
// attributes/instructions, then write back. Without ASM dependency, we verify the
// read+transform+write round-trip completes without exceptions.
TEST(CrasherPatchingTest, CrasherAsm)
{
    auto root = test::getResourcePath("obfuscated/crasher-asm");
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "Sample dir not found: " << root;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource("obfuscated/crasher-asm/" + rel.generic_string());
        if (bytes.empty()) continue;

        ReservedOpcodeRewritingReader reader;
        try {
            auto                                               cf = reader.read(bytes);
            jclassfile::transform::IllegalStrippingTransformer transformer(cf);
            transformer.transform();
            jclassfile::io::ClassFileWriter writer;
            auto                            rewritten = writer.write(cf);
            (void)rewritten;
        }
        catch (const jclassfile::InvalidClassException& e) {
            FAIL() << "InvalidClassException on " << entry.path().filename().string() << ": "
                   << e.what();
        }
        catch (const std::exception& e) {
            FAIL() << "Unexpected error on " << entry.path().filename().string() << ": "
                   << e.what();
        }
    }
}
