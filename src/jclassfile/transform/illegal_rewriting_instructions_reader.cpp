// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/transform/IllegalRewritingInstructionsReader.java

#include "jclassfile/transform/illegal_rewriting_instructions_reader.hpp"

#include <stdexcept>

#include "jclassfile/io/instruction_reader.hpp"
#include "jclassfile/model/constant_pool_constants.hpp"
#include "jclassfile/model/attribute/version_constants.hpp"
#include "jclassfile/model/instruction/basic_instruction.hpp"
#include "jclassfile/model/instruction/int_operand_instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"
#include "jclassfile/model/instruction/reserved_opcodes.hpp"

namespace jclassfile::transform {

using namespace jclassfile::model;
namespace Opcodes         = jclassfile::model::Opcodes;
namespace ReservedOpcodes = jclassfile::model::ReservedOpcodes;

namespace {
const auto NOP_INSN     = std::make_unique<BasicInstruction>(Opcodes::NOP);
const auto ALOAD_0_INSN = std::make_unique<BasicInstruction>(Opcodes::ALOAD_0);
const auto CALOAD_INSN  = std::make_unique<BasicInstruction>(Opcodes::CALOAD);
const auto RETURN_INSN  = std::make_unique<BasicInstruction>(Opcodes::RETURN);
}   // namespace

std::vector<std::unique_ptr<Instruction>> IllegalRewritingInstructionsReader::read(
    int opcode, io::IndexableByteStream& is)
{
    using namespace ReservedOpcodes;

    if ((classVersion_ <= VersionConstants::JAVA8 && opcode == shouldnotreachhere_v8) ||
        (classVersion_ == VersionConstants::JAVA9 && opcode == shouldnotreachhere_v9) ||
        (classVersion_ >= VersionConstants::JAVA11 && opcode == shouldnotreachhere_v11)) {
        rewritten_ = true;
        std::vector<std::unique_ptr<Instruction>> result;
        result.push_back(std::make_unique<BasicInstruction>(Opcodes::NOP));
        return result;
    }

    switch (opcode) {
    case breakpoint:
        rewritten_ = true;
        is.readByte();
        {
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::make_unique<BasicInstruction>(Opcodes::NOP));
            result.push_back(std::make_unique<BasicInstruction>(Opcodes::NOP));
            return result;
        }
    case fast_agetfield:
    case fast_bgetfield:
    case fast_cgetfield:
    case fast_dgetfield:
    case fast_fgetfield:
    case fast_igetfield:
    case fast_lgetfield:
    case fast_sgetfield:
    case nofast_getfield:
        rewritten_ = true;
        {
            auto insn =
                io::InstructionReader::readMemberReferenceInstruction(is, cp_, Opcodes::GETFIELD);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(insn));
            return result;
        }
    case fast_aputfield:
    case fast_bputfield:
    case fast_cputfield:
    case fast_dputfield:
    case fast_fputfield:
    case fast_iputfield:
    case fast_lputfield:
    case fast_sputfield:
    case fast_zputfield:
    case nofast_putfield:
        rewritten_ = true;
        {
            auto insn =
                io::InstructionReader::readMemberReferenceInstruction(is, cp_, Opcodes::PUTFIELD);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(insn));
            return result;
        }
    case fast_aload_0:
    case nofast_aload_0:
        rewritten_ = true;
        {
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::make_unique<BasicInstruction>(Opcodes::ALOAD_0));
            return result;
        }
    case fast_iload:
    case nofast_iload:
        rewritten_ = true;
        {
            auto insn = io::InstructionReader::readXLoad(is, Opcodes::ILOAD);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(insn));
            return result;
        }
    case fast_aldc:
        rewritten_ = true;
        {
            auto insn = io::InstructionReader::readLdc(is, cp_);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(insn));
            return result;
        }
    case fast_aldc_w:
        rewritten_ = true;
        {
            auto insn = io::InstructionReader::readLdcW(is, cp_, Opcodes::LDC_W);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(insn));
            return result;
        }
    case return_register_finalizer:
        rewritten_ = true;
        {
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::make_unique<BasicInstruction>(Opcodes::RETURN));
            return result;
        }
    case fast_invokevfinal:
        rewritten_ = true;
        {
            auto insn = io::InstructionReader::readMemberReferenceInstruction(
                is, cp_, Opcodes::INVOKEVIRTUAL);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(insn));
            return result;
        }
    case fast_linearswitch:
    case fast_binaryswitch:
        rewritten_ = true;
        {
            auto insn = io::InstructionReader::readLookupSwitchInstruction(is);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(insn));
            return result;
        }
    case fast_iaccess_0:
    case fast_aaccess_0:
    case fast_faccess_0:
        rewritten_ = true;
        is.readUnsignedByte();
        {
            auto getfield =
                io::InstructionReader::readMemberReferenceInstruction(is, cp_, Opcodes::GETFIELD);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::make_unique<BasicInstruction>(Opcodes::ALOAD_0));
            result.push_back(std::move(getfield));
            return result;
        }
    case fast_iload2:
        rewritten_ = true;
        {
            auto iload1 = io::InstructionReader::readXLoad(is, Opcodes::ILOAD);
            is.readUnsignedByte();
            auto iload2 = io::InstructionReader::readXLoad(is, Opcodes::ILOAD);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(iload1));
            result.push_back(std::move(iload2));
            return result;
        }
    case fast_icaload:
        rewritten_ = true;
        {
            auto iload = io::InstructionReader::readXLoad(is, Opcodes::ILOAD);
            is.readUnsignedByte();
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(iload));
            result.push_back(std::make_unique<BasicInstruction>(Opcodes::CALOAD));
            return result;
        }
    case invokehandle:
        rewritten_ = true;
        {
            auto insn = io::InstructionReader::readMemberReferenceInstruction(
                is, cp_, Opcodes::INVOKEVIRTUAL);
            std::vector<std::unique_ptr<Instruction>> result;
            result.push_back(std::move(insn));
            return result;
        }
    default: throw std::logic_error("Don't know how to rewrite " + std::to_string(opcode));
    }
}

}   // namespace jclassfile::transform
