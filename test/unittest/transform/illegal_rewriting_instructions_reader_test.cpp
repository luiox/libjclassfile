#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "jclassfile/io/indexable_byte_stream.hpp"
#include "jclassfile/model/attribute/version_constants.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_field_ref.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/instruction/basic_instruction.hpp"
#include "jclassfile/model/instruction/cp_ref_instruction.hpp"
#include "jclassfile/model/instruction/int_operand_instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"
#include "jclassfile/model/instruction/reserved_opcodes.hpp"
#include "jclassfile/transform/illegal_rewriting_instructions_reader.hpp"

using namespace jclassfile;
using namespace jclassfile::io;
using namespace jclassfile::model;
using namespace jclassfile::transform;

namespace {

void populateFieldRefPool(ConstPool& pool, const CpFieldRef*& fieldRefOut)
{

    auto    ownerName    = std::make_unique<CpUtf8>("Owner");
    CpUtf8* ownerNamePtr = ownerName.get();
    pool.add(std::move(ownerName));

    auto    fieldName    = std::make_unique<CpUtf8>("field");
    CpUtf8* fieldNamePtr = fieldName.get();
    pool.add(std::move(fieldName));

    auto    fieldDesc    = std::make_unique<CpUtf8>("I");
    CpUtf8* fieldDescPtr = fieldDesc.get();
    pool.add(std::move(fieldDesc));

    auto     ownerClass    = std::make_unique<CpClass>(ownerNamePtr);
    CpClass* ownerClassPtr = ownerClass.get();
    pool.add(std::move(ownerClass));

    auto        nameType    = std::make_unique<CpNameType>(fieldNamePtr, fieldDescPtr);
    CpNameType* nameTypePtr = nameType.get();
    pool.add(std::move(nameType));

    auto fieldRef = std::make_unique<CpFieldRef>(ownerClassPtr, nameTypePtr);
    fieldRefOut   = fieldRef.get();
    pool.add(std::move(fieldRef));
}

std::vector<ca::u8> bytes(std::initializer_list<ca::u8> init)
{
    return std::vector<ca::u8>(init);
}

}   // namespace

TEST(IllegalRewritingInstructionsReaderTest, RewritesVersionSpecificShouldNotReachHereToNop)
{
    ConstPool           pool;
    auto                data = bytes({});
    IndexableByteStream is(data);

    IllegalRewritingInstructionsReader reader(pool, VersionConstants::JAVA11);
    auto rewritten = reader.read(ReservedOpcodes::shouldnotreachhere_v11, is);

    ASSERT_EQ(rewritten.size(), 1u);
    EXPECT_EQ(rewritten[0]->kind(), InsnKind::Basic);
    EXPECT_EQ(rewritten[0]->getOpcode(), Opcodes::NOP);
    EXPECT_TRUE(reader.wasRewritten());
}

TEST(IllegalRewritingInstructionsReaderTest, RewritesBreakpointToTwoNopsAndConsumesPaddingByte)
{
    ConstPool           pool;
    auto                data = bytes({0x7F});
    IndexableByteStream is(data);

    IllegalRewritingInstructionsReader reader(pool, VersionConstants::JAVA8);
    auto                               rewritten = reader.read(ReservedOpcodes::breakpoint, is);

    ASSERT_EQ(rewritten.size(), 2u);
    EXPECT_EQ(rewritten[0]->getOpcode(), Opcodes::NOP);
    EXPECT_EQ(rewritten[1]->getOpcode(), Opcodes::NOP);
    EXPECT_EQ(is.getAbsoluteIndex(), 1);
    EXPECT_TRUE(reader.wasRewritten());
}

TEST(IllegalRewritingInstructionsReaderTest, RewritesFastGetFieldToGetFieldCpRef)
{
    const CpFieldRef* expectedFieldRef = nullptr;
    ConstPool         pool;
    populateFieldRefPool(pool, expectedFieldRef);
    auto                data = bytes({0x00, 0x06});
    IndexableByteStream is(data);

    IllegalRewritingInstructionsReader reader(pool, VersionConstants::JAVA8);
    auto                               rewritten = reader.read(ReservedOpcodes::fast_agetfield, is);

    ASSERT_EQ(rewritten.size(), 1u);
    ASSERT_EQ(rewritten[0]->kind(), InsnKind::CpRef);
    const auto* cpRef = static_cast<const CpRefInstruction*>(rewritten[0].get());
    EXPECT_EQ(cpRef->getOpcode(), Opcodes::GETFIELD);
    EXPECT_EQ(cpRef->getEntry(), expectedFieldRef);
    EXPECT_TRUE(reader.wasRewritten());
}

TEST(IllegalRewritingInstructionsReaderTest, RewritesFastIload2ToTwoIloads)
{
    auto                data = bytes({0x02, 0x00, 0x05});
    IndexableByteStream is(data);
    ConstPool           pool;

    IllegalRewritingInstructionsReader reader(pool, VersionConstants::JAVA8);
    auto                               rewritten = reader.read(ReservedOpcodes::fast_iload2, is);

    ASSERT_EQ(rewritten.size(), 2u);
    ASSERT_EQ(rewritten[0]->kind(), InsnKind::IntOperand);
    ASSERT_EQ(rewritten[1]->kind(), InsnKind::IntOperand);
    const auto* iload0 = static_cast<const IntOperandInstruction*>(rewritten[0].get());
    const auto* iload1 = static_cast<const IntOperandInstruction*>(rewritten[1].get());
    EXPECT_EQ(iload0->getOpcode(), Opcodes::ILOAD);
    EXPECT_EQ(iload0->getOperand(), 2);
    EXPECT_EQ(iload1->getOpcode(), Opcodes::ILOAD);
    EXPECT_EQ(iload1->getOperand(), 5);
    EXPECT_EQ(is.getAbsoluteIndex(), 3);
    EXPECT_TRUE(reader.wasRewritten());
}

TEST(IllegalRewritingInstructionsReaderTest, ThrowsOnUnknownReservedRewrite)
{
    ConstPool           pool;
    auto                data = bytes({});
    IndexableByteStream is(data);

    IllegalRewritingInstructionsReader reader(pool, VersionConstants::JAVA8);
    EXPECT_THROW((void)reader.read(255, is), std::logic_error);
}
