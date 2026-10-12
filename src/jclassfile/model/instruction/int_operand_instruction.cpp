// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/IntOperandInstruction.java
// 状态：阶段 5b 实现（computeSize 单独拆 .cpp 以减少头文件依赖）

#include "jclassfile/model/instruction/int_operand_instruction.hpp"

namespace jclassfile::model {

int IntOperandInstruction::computeSize() const noexcept
{
    using namespace Opcodes;
    switch (getOpcode()) {
    case RET:
    case BIPUSH:
    case ALOAD:
    case ASTORE:
    case DLOAD:
    case DSTORE:
    case FLOAD:
    case FSTORE:
    case ILOAD:
    case ISTORE:
    case LLOAD:
    case LSTORE:
    case NEWARRAY: return 2;   // opcode + 1 字节操作数
    case GOTO_W:
    case JSR_W: return 5;   // opcode + 4 字节操作数
    default: return 3;      // opcode + 2 字节操作数
    }
}

}   // namespace jclassfile::model
