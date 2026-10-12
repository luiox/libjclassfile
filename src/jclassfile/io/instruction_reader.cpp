// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/InstructionReader.java
// 状态：阶段 5b 实现（510 行 Java 1:1 port，保留全部 opcode case 分支）

#include "jclassfile/io/instruction_reader.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "jclassfile/model/constant/const_ref.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_invoke_dynamic.hpp"
#include "jclassfile/model/constant_pool_constants.hpp"
#include "jclassfile/model/instruction/basic_instruction.hpp"
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
using jclassfile::model::BasicInstruction;
using jclassfile::model::CpRefInstruction;
using jclassfile::model::IincInstruction;
using jclassfile::model::IntOperandInstruction;
using jclassfile::model::LookupSwitchInstruction;
using jclassfile::model::MultiANewArrayInstruction;
using jclassfile::model::TableSwitchInstruction;
using jclassfile::model::WideInstruction;
using jclassfile::model::CpClass;
using jclassfile::model::CpInvokeDynamic;
using jclassfile::model::CpEntry;

namespace Opcodes = jclassfile::model::Opcodes;

std::vector<std::unique_ptr<Instruction>> InstructionReader::read(
    IndexableByteStream& is, const jclassfile::model::ConstPool& pool, int codeLength)
{
    int                                       start = is.getIndex();
    int                                       end   = start + codeLength;
    std::vector<std::unique_ptr<Instruction>> instructions;
    FallbackInstructionReader*                fallback = fallback_.get();
    while (is.getIndex() < end) {
        int opcode = is.readUnsignedByte();
        using namespace Opcodes;
        switch (opcode) {
        // 1 字节无操作数 BasicInstruction 集合
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
        case MONITOREXIT: instructions.push_back(std::make_unique<BasicInstruction>(opcode)); break;
        // 1 字节操作数
        case BIPUSH:
            instructions.push_back(std::make_unique<IntOperandInstruction>(opcode, is.readByte()));
            break;
        case SIPUSH:
            instructions.push_back(std::make_unique<IntOperandInstruction>(opcode, is.readShort()));
            break;
        case ILOAD:
        case LLOAD:
        case FLOAD:
        case DLOAD:
        case ALOAD:
        case ISTORE:
        case LSTORE:
        case FSTORE:
        case DSTORE:
        case ASTORE: instructions.push_back(readXLoad(is, opcode)); break;
        // CP 引用
        case LDC: instructions.push_back(readLdc(is, pool)); break;
        case LDC_W:
        case LDC2_W: instructions.push_back(readLdcW(is, pool, opcode)); break;
        // 2 字节 branch 操作数
        case IFEQ:
        case IFNE:
        case IFLT:
        case IFGE:
        case IFGT:
        case IFLE:
            instructions.push_back(std::make_unique<IntOperandInstruction>(opcode, is.readShort()));
            break;
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
            instructions.push_back(std::make_unique<IntOperandInstruction>(opcode, is.readShort()));
            break;
        case RET:
            instructions.push_back(
                std::make_unique<IntOperandInstruction>(RET, is.readUnsignedByte()));
            break;
        case TABLESWITCH:
        {
            int pos = is.getIndex();
            is.skipBytes((4 - pos & 3));
            int dflt = is.readInt();
            int low  = is.readInt();
            int high = is.readInt();
            // count 保持 int 回绕语义（上游 CAFED00D 的 Java 实现同样回绕，
            // crasher 语料依赖该行为解析极端 low/high 的 junk switch）。
            // 防护点只在两处：负 count 对应 Java NegativeArraySizeException，
            // 显式拒绝而非经 (size_t) 负数触发天文 reserve；reserve 上限按
            // 底层缓冲剩余字节的 1/4 钳制，循环超过缓冲供给时由 readInt 的
            // require 自然抛出终止，内存与循环都有界。刻意不按 code 区段
            // 剩余量封顶——CODE 属性读后按实际绝对位置回定位父流，溢读
            // switch 是语料既定契约。
            int count = high - low + 1;
            if (count < 0) {
                throw std::runtime_error("tableswitch case count negative: " +
                                         std::to_string(count));
            }
            std::vector<int> offsets;
            offsets.reserve(std::min<std::size_t>(static_cast<std::size_t>(count),
                                                  static_cast<std::size_t>(is.readableBytesLeft() / 4)));
            for (int i = 0; i < count; ++i) {
                offsets.push_back(is.readInt());
            }
            auto tswitch =
                std::make_unique<TableSwitchInstruction>(dflt, low, high, std::move(offsets));
            // pos 是 opcode 之后的第一个字节；opcode 位置 = pos - 1
            tswitch->notifyStartPosition(pos - 1);
            instructions.push_back(std::move(tswitch));
            break;
        }
        case LOOKUPSWITCH:
        {
            auto lswitch = readLookupSwitchInstruction(is);
            instructions.push_back(std::move(lswitch));
            break;
        }
        // 字段 / 引用
        case GETSTATIC:
        case PUTSTATIC:
        case GETFIELD:
        case PUTFIELD:
        case INVOKEVIRTUAL:
        case INVOKESPECIAL:
        case INVOKESTATIC:
            instructions.push_back(readMemberReferenceInstruction(is, pool, opcode));
            break;
        case INVOKEINTERFACE:
        {
            int idx = is.readUnsignedShort();
            is.readShort();   // arg-count + 1 padding
            const CpEntry* e = pool.get(idx);
            instructions.push_back(std::make_unique<CpRefInstruction>(opcode, e));
            break;
        }
        case INVOKEDYNAMIC:
        {
            int idx = is.readUnsignedShort();
            is.readShort();   // 2 字节 padding
            const CpEntry* e = pool.get(idx);
            instructions.push_back(std::make_unique<CpRefInstruction>(INVOKEDYNAMIC, e));
            break;
        }
        case NEW:
        case ANEWARRAY:
        case CHECKCAST:
        case INSTANCEOF:
        {
            int            idx = is.readUnsignedShort();
            const CpEntry* e   = pool.get(idx);
            if (e == nullptr || e->tag() != jclassfile::model::CpTag::CLASS) {
                throw std::runtime_error("Expected CpClass at pool index for opcode " +
                                         std::to_string(opcode));
            }
            instructions.push_back(std::make_unique<CpRefInstruction>(opcode, e));
            break;
        }
        case NEWARRAY:
            instructions.push_back(
                std::make_unique<IntOperandInstruction>(NEWARRAY, is.readUnsignedByte()));
            break;
        case IINC:
        {
            int v = is.readUnsignedByte();
            int c = is.readByte();
            instructions.push_back(std::make_unique<IincInstruction>(v, c));
            break;
        }
        case WIDE:
        {
            int type = is.readUnsignedByte();
            using namespace Opcodes;
            std::unique_ptr<Instruction> backing;
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
                backing = std::make_unique<IntOperandInstruction>(type, is.readUnsignedShort());
                break;
            case IINC:
            {
                int v   = is.readUnsignedShort();
                int c   = is.readShort();
                backing = std::make_unique<IincInstruction>(v, c);
                break;
            }
            default:
                throw std::logic_error("Illegal wide instruction type: " + std::to_string(type));
            }
            instructions.push_back(std::make_unique<WideInstruction>(std::move(backing)));
            break;
        }
        case MULTIANEWARRAY:
        {
            int            idx  = is.readUnsignedShort();
            int            dims = is.readUnsignedByte();
            const CpEntry* e    = pool.get(idx);
            if (e == nullptr || e->tag() != jclassfile::model::CpTag::CLASS) {
                throw std::runtime_error("Expected CpClass for multianewarray at pool index");
            }
            instructions.push_back(std::make_unique<MultiANewArrayInstruction>(
                static_cast<const jclassfile::model::CpClass*>(e), dims));
            break;
        }
        case GOTO_W:
        case JSR_W:
            instructions.push_back(std::make_unique<IntOperandInstruction>(opcode, is.readInt()));
            break;
        default: instructions.push_back(std::move(fallback->read(opcode, is).at(0))); break;
        }
    }
    return instructions;
}

std::unique_ptr<Instruction> InstructionReader::readXLoad(IndexableByteStream& is, int opcode)
{
    return std::make_unique<IntOperandInstruction>(opcode, is.readUnsignedByte());
}

std::unique_ptr<Instruction> InstructionReader::readLdc(IndexableByteStream&                is,
                                                        const jclassfile::model::ConstPool& pool)
{
    int            idx = is.readUnsignedByte();
    const CpEntry* e   = pool.get(idx);
    return std::make_unique<CpRefInstruction>(Opcodes::LDC, e);
}

std::unique_ptr<Instruction> InstructionReader::readLdcW(IndexableByteStream&                is,
                                                         const jclassfile::model::ConstPool& pool,
                                                         int                                 opcode)
{
    int            idx = is.readUnsignedShort();
    const CpEntry* e   = pool.get(idx);
    return std::make_unique<CpRefInstruction>(opcode, e);
}

std::unique_ptr<Instruction> InstructionReader::readLookupSwitchInstruction(
    IndexableByteStream& is)
{
    int pos = is.getIndex();
    is.skipBytes((4 - pos & 3));
    int dflt     = is.readInt();
    int keyCount = is.readInt();
    // 防护口径与 tableswitch 一致：负 keyCount 显式拒绝（Java
    // NegativeArraySizeException 对应），reserve 按底层缓冲剩余字节的 1/8
    // 钳制，循环超出缓冲供给由 require 抛出终止。
    if (keyCount < 0) {
        throw std::runtime_error("lookupswitch key count negative: " +
                                 std::to_string(keyCount));
    }
    std::vector<int> keys;
    std::vector<int> offsets;
    keys.reserve(std::min<std::size_t>(static_cast<std::size_t>(keyCount),
                                       static_cast<std::size_t>(is.readableBytesLeft() / 8)));
    offsets.reserve(keys.capacity());
    for (int i = 0; i < keyCount; ++i) {
        keys.push_back(is.readInt());
        offsets.push_back(is.readInt());
    }
    auto lswitch =
        std::make_unique<LookupSwitchInstruction>(dflt, std::move(keys), std::move(offsets));
    lswitch->notifyStartPosition(pos - 1);
    return lswitch;
}

std::unique_ptr<Instruction> InstructionReader::readMemberReferenceInstruction(
    IndexableByteStream& is, const jclassfile::model::ConstPool& pool, int opcode)
{
    int            idx = is.readUnsignedShort();
    const CpEntry* e   = pool.get(idx);
    return std::make_unique<CpRefInstruction>(opcode, e);
}

}   // namespace jclassfile::io
