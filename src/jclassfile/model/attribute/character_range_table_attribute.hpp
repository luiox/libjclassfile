// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/CharacterRangeTableAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"

namespace jclassfile::model {

class CharacterRangeTableAttribute : public Attribute
{
public:
    class CharacterRangeInfo
    {
    public:
        static constexpr int FLAG_STATEMENT       = 0x0001;
        static constexpr int FLAG_BLOCK           = 0x0002;
        static constexpr int FLAG_ASSIGNMENT      = 0x0004;
        static constexpr int FLAG_FLOW_CONTROLLER = 0x0008;
        static constexpr int FLAG_FLOW_TARGET     = 0x0010;
        static constexpr int FLAG_INVOKE          = 0x0020;
        static constexpr int FLAG_CREATE          = 0x0040;
        static constexpr int FLAG_BRANCH_TRUE     = 0x0080;
        static constexpr int FLAG_BRANCH_FALSE    = 0x0100;

        CharacterRangeInfo(int startPc, int endPc, int characterRangeStart, int characterRangeEnd,
                           int flags) noexcept
            : startPc_(startPc)
            , endPc_(endPc)
            , characterRangeStart_(characterRangeStart)
            , characterRangeEnd_(characterRangeEnd)
            , flags_(flags)
        {}

        [[nodiscard]] int startPc() const noexcept { return startPc_; }
        void              setStartPc(int startPc) noexcept { startPc_ = startPc; }

        [[nodiscard]] int endPc() const noexcept { return endPc_; }
        void              setEndPc(int endPc) noexcept { endPc_ = endPc; }

        [[nodiscard]] int characterRangeStart() const noexcept { return characterRangeStart_; }
        void              setCharacterRangeStart(int characterRangeStart) noexcept
        {
            characterRangeStart_ = characterRangeStart;
        }
        void setCharacterRangeStart(int line, int column) noexcept
        {
            setCharacterRangeStart((line << 10) + column);
        }

        [[nodiscard]] int characterRangeEnd() const noexcept { return characterRangeEnd_; }
        void              setCharacterRangeEnd(int characterRangeEnd) noexcept
        {
            characterRangeEnd_ = characterRangeEnd;
        }
        void setCharacterRangeEnd(int line, int column) noexcept
        {
            setCharacterRangeEnd((line << 10) + column);
        }

        [[nodiscard]] int flags() const noexcept { return flags_; }
        void              setFlags(int flags) noexcept { flags_ = flags; }

        [[nodiscard]] bool equals(const CharacterRangeInfo& other) const noexcept
        {
            return startPc_ == other.startPc_ && endPc_ == other.endPc_ &&
                   characterRangeStart_ == other.characterRangeStart_ &&
                   characterRangeEnd_ == other.characterRangeEnd_ && flags_ == other.flags_;
        }

        [[nodiscard]] int hashCode() const noexcept
        {
            int result = startPc_;
            result     = 31 * result + endPc_;
            result     = 31 * result + characterRangeStart_;
            result     = 31 * result + characterRangeEnd_;
            result     = 31 * result + flags_;
            return result;
        }

    private:
        int startPc_;
        int endPc_;
        int characterRangeStart_;
        int characterRangeEnd_;
        int flags_;
    };

    CharacterRangeTableAttribute(const CpUtf8*                   name,
                                 std::vector<CharacterRangeInfo> characterRangeTable)
        : Attribute(name)
        , characterRangeTable_(std::move(characterRangeTable))
    {}

    [[nodiscard]] const std::vector<CharacterRangeInfo>& characterRangeTable() const noexcept
    {
        return characterRangeTable_;
    }
    void setCharacterRangeTable(std::vector<CharacterRangeInfo> characterRangeTable) noexcept
    {
        characterRangeTable_ = std::move(characterRangeTable);
    }

    [[nodiscard]] int computeInternalLength() const noexcept override
    {
        // 头部 u2 条目数 + 每条目 14 字节（上游 3e814f0a：原版 Java 误写为
        // 2*(14n)，与 attribute_writer 实际写出的 2+14n 不一致，读回即长度漂移）。
        return 2 + 14 * static_cast<int>(characterRangeTable_.size());
    }

private:
    std::vector<CharacterRangeInfo> characterRangeTable_;
};

}   // namespace jclassfile::model
