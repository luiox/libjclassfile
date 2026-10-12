// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/LookupSwitchInstruction.java
// 状态：阶段 5b 实现
// 备注：lookupswitch 指令（含 padding 计算）。
//       padding 公式与 Java 完全一致：padding = 3 - (position & 3)。
//       computeSize 在 padding 未设置时抛 std::logic_error（与原版 IllegalStateException 一致）。

#pragma once

#include <stdexcept>
#include <vector>

#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"

namespace jclassfile::model {

class LookupSwitchInstruction final : public Instruction
{
public:
    LookupSwitchInstruction(int dflt, std::vector<int> keys, std::vector<int> offsets) noexcept
        : Instruction(Opcodes::LOOKUPSWITCH)
        , padding_(-1)
        , dflt_(dflt)
        , keys_(std::move(keys))
        , offsets_(std::move(offsets))
    {}

    [[nodiscard]] InsnKind kind() const noexcept override { return InsnKind::LookupSwitch; }

    [[nodiscard]] int getDefault() const noexcept { return dflt_; }
    void              setDefault(int dflt) noexcept { dflt_ = dflt; }

    [[nodiscard]] const std::vector<int>& getKeys() const noexcept { return keys_; }
    void setKeys(std::vector<int> keys) noexcept { keys_ = std::move(keys); }

    [[nodiscard]] const std::vector<int>& getOffsets() const noexcept { return offsets_; }
    void setOffsets(std::vector<int> offsets) noexcept { offsets_ = std::move(offsets); }

    [[nodiscard]] int getPadding() const noexcept { return padding_; }

    void notifyStartPosition(int position) noexcept override
    {
        // padding = 3 - (position & 3)
        // 边界：position & 3 == 3 时 padding = 0；原版注释说明 4 字节对齐公式
        padding_ = 3 - (position & 3);
    }

    [[nodiscard]] int getStartPosition() const noexcept override { return 0; /* 由调用方记录 */ }

    [[nodiscard]] int computeSize() const noexcept override
    {
        if (padding_ < 0) {
            return -1;   // 调用方负责先 notifyStartPosition
        }
        return 1 + padding_ + 4 + 4 + 4 * static_cast<int>(keys_.size()) +
               4 * static_cast<int>(offsets_.size());
    }

    [[nodiscard]] bool equals(const Instruction& other) const noexcept override
    {
        if (other.kind() != InsnKind::LookupSwitch) return false;
        const auto& that = static_cast<const LookupSwitchInstruction&>(other);
        return padding_ == that.padding_ && dflt_ == that.dflt_ && keys_ == that.keys_ &&
               offsets_ == that.offsets_;
    }

    [[nodiscard]] std::string toString() const override
    {
        std::string s = Instruction::toString() + " ";
        for (std::size_t i = 0; i < keys_.size(); ++i) {
            if (i > 0) s += ", ";
            s += std::to_string(keys_[i]) + "=" + std::to_string(offsets_[i]);
        }
        s += " default=" + std::to_string(dflt_);
        return s;
    }

private:
    int              padding_;
    int              dflt_;
    std::vector<int> keys_;
    std::vector<int> offsets_;
};

}   // namespace jclassfile::model
