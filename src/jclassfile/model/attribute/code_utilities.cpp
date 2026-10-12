// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/CodeUtilities.java

#include "jclassfile/model/attribute/code_utilities.hpp"

#include <memory>

#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"

namespace jclassfile::model {

namespace {
class Wrapper final : public CodeUtilities
{
public:
    explicit Wrapper(const std::vector<const Instruction*>& insns)
        : insns_(insns)
    {}
    const std::vector<const Instruction*>& instructions() const noexcept override { return insns_; }

private:
    const std::vector<const Instruction*>& insns_;
};
}   // namespace

bool CodeUtilities::isBranch(const Instruction* instruction) noexcept
{
    int op = instruction->getOpcode();
    return (op >= Opcodes::IFEQ && op <= Opcodes::JSR) || op == Opcodes::GOTO_W ||
           op == Opcodes::IFNULL || op == Opcodes::IFNONNULL;
}

bool CodeUtilities::isTerminalOrAlwaysTakeFlowControl(const Instruction* instruction) noexcept
{
    int op = instruction->getOpcode();
    switch (op) {
    case Opcodes::IRETURN:
    case Opcodes::LRETURN:
    case Opcodes::FRETURN:
    case Opcodes::DRETURN:
    case Opcodes::ARETURN:
    case Opcodes::RETURN:
    case Opcodes::ATHROW:
    case Opcodes::TABLESWITCH:
    case Opcodes::LOOKUPSWITCH:
    case Opcodes::GOTO:
    case Opcodes::GOTO_W:
    case Opcodes::JSR: return true;
    default: return false;
    }
}

std::unique_ptr<CodeUtilities> CodeUtilities::wrap(
    const std::vector<const Instruction*>& instructions)
{
    return std::make_unique<Wrapper>(instructions);
}

}   // namespace jclassfile::model
