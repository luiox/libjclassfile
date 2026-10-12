// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/Instruction.java
// 状态：阶段 5b 完整实现（替换阶段 3 stub）
// 备注：
//   - Java sealed abstract permits 8 子类：Basic / CpRef / Iinc / IntOperand /
//     LookupSwitch / MultiANewArray / TableSwitch / Wide。
//   - C++ 无 sealed 关键字：abstract 基类 + 注释说明 "sealed equivalent"，
//     与项目内 ConstRef 等 sealed abstract 模式一致。
//   - 项目禁用 RTTI：用 `InsnKind kind()` 枚举替代 Java `instanceof` 做类型分派。
//     equals() 子类实现先比 kind 再比 opcode 再比子类特定数据。
//   - 虚方法表（无 RTTI 替代品）：
//     * kind() — 类型分派枚举（替代 instanceof）
//     * getOpcode() / setOpcode() — opcode 字段读写
//     * computeSize() — 指令字节长度（默认 1 = 仅 opcode）
//     * cpAccesses() — 该指令引用的 CpEntry 列表（默认空，BasicInstruction 等不重写）
//     * notifyStartPosition(int) / getStartPosition() — switch 指令 padding 计算
//     * equals(const Instruction&) — 值相等（kind + opcode + 子类数据）
//     * toString() — 优先用 OpcodeUtil name

#pragma once

#include <string>
#include <vector>

#include "jclassfile/util/opcode_util.hpp"

namespace jclassfile::model {

class CpEntry;

// 替代 Java `instanceof` 做类型分派（项目禁用 RTTI 禁用 dynamic_cast）。
enum class InsnKind
{
    Basic,
    CpRef,
    Iinc,
    IntOperand,
    LookupSwitch,
    MultiANewArray,
    TableSwitch,
    Wide,
};

class Instruction
{
public:
    virtual ~Instruction() = default;

    Instruction(const Instruction&)            = delete;
    Instruction& operator=(const Instruction&) = delete;

    [[nodiscard]] virtual InsnKind kind() const noexcept = 0;

    [[nodiscard]] int getOpcode() const noexcept { return opcode_; }
    void              setOpcode(int opcode) noexcept { opcode_ = opcode; }

    [[nodiscard]] virtual int computeSize() const noexcept = 0;

    [[nodiscard]] virtual std::vector<const CpEntry*> cpAccesses() const { return {}; }

    // Default 无操作；LookupSwitch / TableSwitch 重写以计算 padding。
    virtual void notifyStartPosition(int /*position*/) {}

    [[nodiscard]] virtual int getStartPosition() const noexcept { return 0; }

    [[nodiscard]] virtual bool equals(const Instruction& other) const noexcept
    {
        return kind() == other.kind() && opcode_ == other.opcode_;
    }

    [[nodiscard]] virtual std::string toString() const
    {
        const char* name = util::OpcodeUtil::getOpcodeName(opcode_);
        return name ? std::string(name) : std::string("opcode=") + std::to_string(opcode_);
    }

protected:
    explicit Instruction(int opcode) noexcept
        : opcode_(opcode)
    {}

private:
    int opcode_;
};

}   // namespace jclassfile::model
