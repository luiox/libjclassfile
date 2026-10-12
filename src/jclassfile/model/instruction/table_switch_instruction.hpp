// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/TableSwitchInstruction.java
// 状态：阶段 5b 实现
// 备注：tableswitch 指令（含 padding 计算）。
//       padding 公式与 Java 完全一致：padding = 3 - (position & 3)。
//       computeSize 在 padding 未设置时返回 -1（Java 抛 IllegalStateException，C++ 选择 -1
//       让调用方在写入前必先 notifyStartPosition，写入器会先 skip padding 再触发 size 校验）。

#pragma once

#include <string>
#include <vector>

#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"

namespace jclassfile::model {

class TableSwitchInstruction final : public Instruction
{
public:
    TableSwitchInstruction(int dflt, int low, int high, std::vector<int> offsets) noexcept
        : Instruction(Opcodes::TABLESWITCH)
        , padding_(-1)
        , dflt_(dflt)
        , low_(low)
        , high_(high)
        , offsets_(std::move(offsets))
    {}

    [[nodiscard]] InsnKind kind() const noexcept override { return InsnKind::TableSwitch; }

    [[nodiscard]] int getDefault() const noexcept { return dflt_; }
    void              setDefault(int dflt) noexcept { dflt_ = dflt; }

    [[nodiscard]] int getLow() const noexcept { return low_; }
    void              setLow(int low) noexcept { low_ = low; }

    [[nodiscard]] int getHigh() const noexcept { return high_; }
    void              setHigh(int high) noexcept { high_ = high; }

    [[nodiscard]] const std::vector<int>& getOffsets() const noexcept { return offsets_; }
    void setOffsets(std::vector<int> offsets) noexcept { offsets_ = std::move(offsets); }

    [[nodiscard]] int getPadding() const noexcept { return padding_; }

    void notifyStartPosition(int position) noexcept override
    {
        // padding = 3 - (position & 3)，与原版公式一致
        padding_ = 3 - (position & 3);
    }

    [[nodiscard]] int computeSize() const noexcept override
    {
        if (padding_ < 0) {
            return -1;
        }
        return 1 + padding_ + 4 + 4 + 4 + 4 * static_cast<int>(offsets_.size());
    }

    [[nodiscard]] bool equals(const Instruction& other) const noexcept override
    {
        if (other.kind() != InsnKind::TableSwitch) return false;
        const auto& that = static_cast<const TableSwitchInstruction&>(other);
        return padding_ == that.padding_ && dflt_ == that.dflt_ && low_ == that.low_ &&
               high_ == that.high_ && offsets_ == that.offsets_;
    }

    [[nodiscard]] std::string toString() const override
    {
        std::string s = Instruction::toString() + " range=[" + std::to_string(low_) + " --> " +
                        std::to_string(high_) + "] offsets=[";
        for (std::size_t i = 0; i < offsets_.size(); ++i) {
            if (i > 0) s += ", ";
            s += std::to_string(offsets_[i]);
        }
        s += "] default=" + std::to_string(dflt_);
        return s;
    }

private:
    int              padding_;
    int              dflt_;
    int              low_;
    int              high_;
    std::vector<int> offsets_;
};

}   // namespace jclassfile::model
