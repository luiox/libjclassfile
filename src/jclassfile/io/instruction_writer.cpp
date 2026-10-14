// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/InstructionWriter.java
// 状态：阶段 5b 实现

#include "jclassfile/io/instruction_writer.hpp"

#include <stdexcept>
#include <utility>

#include "jclassfile/invalid_class_exception.hpp"
#include "jclassfile/model/constant/const_ref.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/constant_pool_constants.hpp"
#include "jclassfile/model/descriptor.hpp"
#include "jclassfile/model/instruction/cp_ref_instruction.hpp"
#include "jclassfile/model/instruction/iinc_instruction.hpp"
#include "jclassfile/model/instruction/int_operand_instruction.hpp"
#include "jclassfile/model/instruction/lookup_switch_instruction.hpp"
#include "jclassfile/model/instruction/multi_a_new_array_instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"
#include "jclassfile/model/instruction/table_switch_instruction.hpp"
#include "jclassfile/model/instruction/wide_instruction.hpp"

namespace jclassfile::io {

using jclassfile::model::Instruction;
using jclassfile::model::CpRefInstruction;
using jclassfile::model::IincInstruction;
using jclassfile::model::IntOperandInstruction;
using jclassfile::model::LookupSwitchInstruction;
using jclassfile::model::MultiANewArrayInstruction;
using jclassfile::model::TableSwitchInstruction;
using jclassfile::model::WideInstruction;
using jclassfile::model::ConstRef;
using jclassfile::model::Descriptor;
using jclassfile::model::CpNameType;

namespace Opcodes = jclassfile::model::Opcodes;

namespace {

// CpRef 指令写路径前置校验：null entry 或错 tag 在写出前抛 InvalidClassException
// （可捕获、带 opcode 上下文），替代旧实现直通硬件异常的 SEGV/UB。
const jclassfile::model::CpEntry& requireCpRefEntry(const CpRefInstruction* cpref, int opcode)
{
    const jclassfile::model::CpEntry* entry = cpref->getEntry();
    if (entry == nullptr) {
        throw InvalidClassException("Instruction writes a null constant-pool reference for opcode " +
                                    std::to_string(opcode));
    }
    return *entry;
}

}   // namespace

std::vector<ca::u8> InstructionWriter::writeCode(const std::vector<const Instruction*>& list)
{
    util::GrowingByteBuffer    buffer;
    FallbackInstructionWriter* fallback = fallback_;
    for (const Instruction* instruction : list) {
        int expectedSize = instruction->computeSize();
        int startPos     = buffer.position();
        int opcode       = instruction->getOpcode();
        buffer.put(opcode & 0xFF);
        using namespace Opcodes;
        switch (opcode) {
        // 无操作数 BasicInstruction（与 reader 列表完全镜像）
        case NOP:
        case ACONST_NULL:
        case ICONST_M1:
        case ICONST_0:
        case ICONST_1:
        case ICONST_2:
        case ICONST_3:
        case ICONST_4:
        case ICONST_5:
        case LCONST_0:
        case LCONST_1:
        case FCONST_0:
        case FCONST_1:
        case FCONST_2:
        case DCONST_0:
        case DCONST_1:
        case ILOAD_0:
        case ILOAD_1:
        case ILOAD_2:
        case ILOAD_3:
        case LLOAD_0:
        case LLOAD_1:
        case LLOAD_2:
        case LLOAD_3:
        case FLOAD_0:
        case FLOAD_1:
        case FLOAD_2:
        case FLOAD_3:
        case DLOAD_0:
        case DLOAD_1:
        case DLOAD_2:
        case DLOAD_3:
        case ALOAD_0:
        case ALOAD_1:
        case ALOAD_2:
        case ALOAD_3:
        case IALOAD:
        case LALOAD:
        case FALOAD:
        case DALOAD:
        case AALOAD:
        case BALOAD:
        case CALOAD:
        case SALOAD:
        case ISTORE_0:
        case ISTORE_1:
        case ISTORE_2:
        case ISTORE_3:
        case LSTORE_0:
        case LSTORE_1:
        case LSTORE_2:
        case LSTORE_3:
        case FSTORE_0:
        case FSTORE_1:
        case FSTORE_2:
        case FSTORE_3:
        case DSTORE_0:
        case DSTORE_1:
        case DSTORE_2:
        case DSTORE_3:
        case ASTORE_0:
        case ASTORE_1:
        case ASTORE_2:
        case ASTORE_3:
        case IASTORE:
        case LASTORE:
        case FASTORE:
        case DASTORE:
        case AASTORE:
        case BASTORE:
        case CASTORE:
        case SASTORE:
        case POP:
        case POP2:
        case DUP:
        case DUP_X1:
        case DUP_X2:
        case DUP2:
        case DUP2_X1:
        case DUP2_X2:
        case SWAP:
        case IADD:
        case LADD:
        case FADD:
        case DADD:
        case ISUB:
        case LSUB:
        case FSUB:
        case DSUB:
        case IMUL:
        case LMUL:
        case FMUL:
        case DMUL:
        case IDIV:
        case LDIV:
        case FDIV:
        case DDIV:
        case IREM:
        case LREM:
        case FREM:
        case DREM:
        case INEG:
        case LNEG:
        case FNEG:
        case DNEG:
        case ISHL:
        case LSHL:
        case ISHR:
        case LSHR:
        case IUSHR:
        case LUSHR:
        case IAND:
        case LAND:
        case IOR:
        case LOR:
        case IXOR:
        case LXOR:
        case I2L:
        case I2F:
        case I2D:
        case L2I:
        case L2F:
        case L2D:
        case F2I:
        case F2L:
        case F2D:
        case D2I:
        case D2L:
        case D2F:
        case I2B:
        case I2C:
        case I2S:
        case LCMP:
        case FCMPL:
        case FCMPG:
        case DCMPL:
        case DCMPG:
        case IRETURN:
        case LRETURN:
        case FRETURN:
        case DRETURN:
        case ARETURN:
        case RETURN:
        case ARRAYLENGTH:
        case ATHROW:
        case MONITORENTER:
        case MONITOREXIT: break;
        case BIPUSH:
        {
            const auto* i = static_cast<const IntOperandInstruction*>(instruction);
            buffer.put(i->getOperand());
            break;
        }
        case SIPUSH:
        case IFEQ:
        case IFNE:
        case IFLT:
        case IFGE:
        case IFGT:
        case IFLE:
        case IF_ICMPEQ:
        case IF_ICMPNE:
        case IF_ICMPLT:
        case IF_ICMPGE:
        case IF_ICMPGT:
        case IF_ICMPLE:
        case IF_ACMPEQ:
        case IF_ACMPNE:
        case GOTO:
        case JSR:
        case IFNULL:
        case IFNONNULL:
        {
            const auto* i = static_cast<const IntOperandInstruction*>(instruction);
            buffer.putShort(i->getOperand());
            break;
        }
        case ILOAD:
        case LLOAD:
        case FLOAD:
        case DLOAD:
        case ALOAD:
        case ISTORE:
        case LSTORE:
        case FSTORE:
        case DSTORE:
        case ASTORE:
        case RET:
        case NEWARRAY:
        {
            const auto* i = static_cast<const IntOperandInstruction*>(instruction);
            buffer.put(i->getOperand() & 0xFF);
            break;
        }
        case LDC:
        {
            const auto* cpref = static_cast<const CpRefInstruction*>(instruction);
            buffer.put(requireCpRefEntry(cpref, opcode).index() & 0xFF);
            break;
        }
        case LDC_W:
        case LDC2_W:
        case GETSTATIC:
        case PUTSTATIC:
        case GETFIELD:
        case PUTFIELD:
        case INVOKEVIRTUAL:
        case INVOKESPECIAL:
        case INVOKESTATIC:
        case NEW:
        case ANEWARRAY:
        case CHECKCAST:
        case INSTANCEOF:
        {
            const auto* cpref = static_cast<const CpRefInstruction*>(instruction);
            buffer.putShort(requireCpRefEntry(cpref, opcode).index() & 0xFFFF);
            break;
        }
        case INVOKEINTERFACE:
        {
            const auto* cpref = static_cast<const CpRefInstruction*>(instruction);
            const auto& entry = requireCpRefEntry(cpref, opcode);
            // 非 ConstRef 族的条目（CpClass/Utf8 等）在旧实现下被 static_cast 后
            // 连解三级（nameType/type 全是垃圾解引用，UB），前置拒绝；ConstRef
            // 族内不区分 Field/Method/InterfaceMethodRef（InstructionRoundTrip
            // 契约：writer 只要求可解析出 nameType）。
            switch (entry.tag()) {
            case jclassfile::model::CpTag::FIELD_REF:
            case jclassfile::model::CpTag::METHOD_REF:
            case jclassfile::model::CpTag::INTERFACE_METHOD_REF: break;
            default:
                throw InvalidClassException(
                    "INVOKEINTERFACE references a non-constant-ref constant (tag " +
                    std::to_string(static_cast<int>(entry.tag())) + ")");
            }
            const auto&       ref = static_cast<const ConstRef&>(entry);
            const CpNameType* nt  = ref.nameType();
            if (nt == nullptr || nt->type() == nullptr) {
                throw InvalidClassException(
                    "INVOKEINTERFACE references a constant ref without name/type");
            }
            buffer.putShort(entry.index() & 0xFFFF);
            // arg-count = 显式参数槽 + 1 (this)
            int argSize = Descriptor(nt->type()->text()).getParameterSize() + 1;
            buffer.put(argSize);
            buffer.put(0);
            break;
        }
        case IINC:
        {
            const auto* iinc = static_cast<const IincInstruction*>(instruction);
            buffer.put(iinc->getVar() & 0xFF);
            buffer.put(iinc->getIncrement());
            break;
        }
        case TABLESWITCH:
        {
            buffer.skip(4 - (startPos + 1) & 3);
            const auto* tsw = static_cast<const TableSwitchInstruction*>(instruction);
            buffer.putInt(tsw->getDefault());
            buffer.putInt(tsw->getLow());
            buffer.putInt(tsw->getHigh());
            for (int branch : tsw->getOffsets()) {
                buffer.putInt(branch);
            }
            break;
        }
        case LOOKUPSWITCH:
        {
            buffer.skip(4 - (startPos + 1) & 3);
            const auto* lsw = static_cast<const LookupSwitchInstruction*>(instruction);
            buffer.putInt(lsw->getDefault());
            const auto& keys    = lsw->getKeys();
            const auto& offsets = lsw->getOffsets();
            int         count   = static_cast<int>(keys.size());
            buffer.putInt(count);
            for (int i = 0; i < count; ++i) {
                buffer.putInt(keys[i]);
                buffer.putInt(offsets[i]);
            }
            break;
        }
        case INVOKEDYNAMIC:
        {
            const auto* cpref = static_cast<const CpRefInstruction*>(instruction);
            buffer.putShort(requireCpRefEntry(cpref, opcode).index() & 0xFFFF);
            buffer.putShort(0);
            break;
        }
        case WIDE:
        {
            const auto*        wide    = static_cast<const WideInstruction*>(instruction);
            const Instruction* backing = wide->getBacking();
            int                type    = backing->getOpcode();
            buffer.put(type & 0xFF);
            using namespace Opcodes;
            switch (type) {
            case ILOAD:
            case FLOAD:
            case ALOAD:
            case LLOAD:
            case DLOAD:
            case ISTORE:
            case FSTORE:
            case ASTORE:
            case LSTORE:
            case DSTORE:
            case RET:
            {
                const auto* i = static_cast<const IntOperandInstruction*>(backing);
                buffer.putShort(i->getOperand() & 0xFFFF);
                break;
            }
            case IINC:
            {
                const auto* iinc = static_cast<const IincInstruction*>(backing);
                buffer.putShort(iinc->getVar() & 0xFFFF);
                buffer.putShort(iinc->getIncrement());
                break;
            }
            default:
                throw std::logic_error("Illegal wide instruction type: " + std::to_string(type));
            }
            break;
        }
        case MULTIANEWARRAY:
        {
            const auto* multi = static_cast<const MultiANewArrayInstruction*>(instruction);
            if (multi->getDescriptor() == nullptr) {
                throw InvalidClassException("MULTIANEWARRAY writes a null class reference");
            }
            buffer.putShort(multi->getDescriptor()->index() & 0xFFFF);
            buffer.put(multi->getDimensions());
            break;
        }
        case GOTO_W:
        case JSR_W:
        {
            const auto* i = static_cast<const IntOperandInstruction*>(instruction);
            buffer.putInt(i->getOperand());
            break;
        }
        default: fallback->write(*instruction, buffer); break;
        }

        int endPos = buffer.position();
        int size   = endPos - startPos;
        if (size != expectedSize) {
            throw std::logic_error(
                "Instruction size expectation mismatch: " + instruction->toString() +
                "\n - expected=" + std::to_string(expectedSize) +
                ", actual=" + std::to_string(size));
        }
    }
    // 等价于 Java ByteBuffer.array() + Arrays.copyOf(content, position)
    // 先物化再取迭代器：两次 unwrap() 会产生两个不同临时容器，跨容器迭代器
    // 比较在 MSVC debug 下断言失败、release 下为 UB。
    auto bytes = buffer.unwrap();
    return std::vector<ca::u8>(bytes.begin(), bytes.begin() + buffer.position());
}

}   // namespace jclassfile::io
