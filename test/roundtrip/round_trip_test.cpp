#include <gtest/gtest.h>

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

// Byte-level equality: read + strip + write produces identical bytes
void testEquality(const std::string& relPath)
{
    auto root = test::getResourcePath(relPath);
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << relPath << " not found";
        return;
    }
    int count = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource(relPath + "/" + rel.generic_string());
        if (bytes.empty()) continue;
        ++count;

        try {
            auto cf = jclassfile::io::ClassFileReader().read(bytes);
            jclassfile::transform::IllegalStrippingTransformer(cf).transform();
            auto out = jclassfile::io::ClassFileWriter().write(cf);
            EXPECT_EQ(bytes.size(), out.size()) << entry.path();
            EXPECT_EQ(bytes, out) << entry.path();
        }
        catch (const jclassfile::InvalidClassException& e) {
            FAIL() << entry.path().string() << ": " << e.what();
        }
        catch (const std::exception& e) {
            FAIL() << entry.path().string() << ": " << e.what();
        }
    }
    if (count == 0) GTEST_SKIP() << "no .class files in " << relPath;
}

// No-crash: read + strip + write must not throw (output bytes may differ due to
// illegal content removal — matches CAFED00D CrasherPatchingTest behavior)
void testNoCrash(const std::string& relPath, bool useReservedReader = false)
{
    auto root = test::getResourcePath(relPath);
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << relPath << " not found";
        return;
    }
    int count = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource(relPath + "/" + rel.generic_string());
        if (bytes.empty()) continue;
        ++count;

        try {
            jclassfile::model::ClassFile cf = [&]() {
                if (useReservedReader) {
                    ReservedOpcodeRewritingReader reader;
                    return reader.read(bytes);
                }
                return jclassfile::io::ClassFileReader().read(bytes);
            }();
            jclassfile::transform::IllegalStrippingTransformer(cf).transform();
            auto out = jclassfile::io::ClassFileWriter().write(cf);
            (void)out;
        }
        catch (const jclassfile::InvalidClassException& e) {
            FAIL() << entry.path().string() << ": " << e.what();
        }
        catch (const std::exception& e) {
            FAIL() << entry.path().string() << ": " << e.what();
        }
    }
    if (count == 0) GTEST_SKIP() << "no .class files in " << relPath;
}

}   // namespace

// Byte-level equality tests (clean samples with no illegal content).
// Mirrors CAFED00D EqualityIOTest, which asserts read→strip→write byte-equality
// only on samples/normal. Obfuscated samples (proguard/crasher/misc/oak) carry
// illegal content (e.g. junk LocalVariableTable names, bad annotations) that the
// IllegalStrippingTransformer legitimately removes, so they go in the no-crash
// bucket below rather than the equality bucket.
TEST(RoundTripTest, NormalJavac)
{
    testEquality("normal/javac");
}
TEST(RoundTripTest, NormalAnnos)
{
    testEquality("normal/annos");
}
TEST(RoundTripTest, NormalMisc)
{
    testEquality("normal/misc");
}
TEST(RoundTripTest, NormalModules)
{
    testEquality("normal/modules");
}

// No-crash tests (samples with illegal content that gets stripped/rewritten)
TEST(RoundTripTest, ObfuscatedProguard)
{
    testNoCrash("obfuscated/proguard");
}
TEST(RoundTripTest, ObfuscatedCrasherAsm)
{
    testNoCrash("obfuscated/crasher-asm", true);
}
TEST(RoundTripTest, ObfuscatedCrasherDude)
{
    testNoCrash("obfuscated/crasher-dude", true);
}
TEST(RoundTripTest, ObfuscatedMisc)
{
    testNoCrash("obfuscated/misc", true);
}
TEST(RoundTripTest, Oak)
{
    testNoCrash("oak");
}
