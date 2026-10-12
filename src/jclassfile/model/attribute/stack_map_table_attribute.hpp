// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/StackMapTableAttribute.java

#pragma once

#include <memory>
#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/attribute/stack_map_table_constants.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class StackMapTableAttribute : public Attribute, public StackMapTableConstants
{
public:
    class TypeInfo : public behavior::CpAccessor
    {
    public:
        [[nodiscard]] virtual int getTag() const noexcept = 0;

        [[nodiscard]] virtual int getLength() const noexcept
        {
            // u1: tag
            return 1;
        }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    };

    class TopVariableInfo : public TypeInfo
    {
    public:
        [[nodiscard]] int getTag() const noexcept override { return ITEM_TOP; }
    };

    class IntegerVariableInfo : public TypeInfo
    {
    public:
        [[nodiscard]] int getTag() const noexcept override { return ITEM_INTEGER; }
    };

    class FloatVariableInfo : public TypeInfo
    {
    public:
        [[nodiscard]] int getTag() const noexcept override { return ITEM_FLOAT; }
    };

    class DoubleVariableInfo : public TypeInfo
    {
    public:
        [[nodiscard]] int getTag() const noexcept override { return ITEM_DOUBLE; }
    };

    class LongVariableInfo : public TypeInfo
    {
    public:
        [[nodiscard]] int getTag() const noexcept override { return ITEM_LONG; }
    };

    class NullVariableInfo : public TypeInfo
    {
    public:
        [[nodiscard]] int getTag() const noexcept override { return ITEM_NULL; }
    };

    class UninitializedThisVariableInfo : public TypeInfo
    {
    public:
        [[nodiscard]] int getTag() const noexcept override { return ITEM_UNINITIALIZED_THIS; }
    };

    class ObjectVariableInfo : public TypeInfo
    {
    public:
        explicit ObjectVariableInfo(const CpClass* classEntry) noexcept
            : classEntry_(classEntry)
        {}

        [[nodiscard]] int getTag() const noexcept override { return ITEM_OBJECT; }
        [[nodiscard]] int getLength() const noexcept override { return 1 + 2; }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

        [[nodiscard]] const CpClass* classEntry() const noexcept { return classEntry_; }
        void setClassEntry(const CpClass* classEntry) noexcept { classEntry_ = classEntry; }

    private:
        const CpClass* classEntry_;
    };

    class UninitializedVariableInfo : public TypeInfo
    {
    public:
        explicit UninitializedVariableInfo(int offset) noexcept
            : offset_(offset)
        {}

        [[nodiscard]] int getTag() const noexcept override { return ITEM_UNINITIALIZED; }
        [[nodiscard]] int getLength() const noexcept override { return 1 + 2; }

        [[nodiscard]] int offset() const noexcept { return offset_; }
        void              setOffset(int offset) noexcept { offset_ = offset; }

    private:
        int offset_;
    };

    class StackMapFrame : public behavior::CpAccessor
    {
    public:
        explicit StackMapFrame(int offsetDelta) noexcept
            : offsetDelta_(offsetDelta)
        {}

        [[nodiscard]] virtual int getFrameType() const noexcept = 0;

        [[nodiscard]] virtual int getLength() const noexcept
        {
            // u1 frame_type
            return 1;
        }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

        [[nodiscard]] int offsetDelta() const noexcept { return offsetDelta_; }
        void              setOffsetDelta(int offsetDelta) noexcept { offsetDelta_ = offsetDelta; }

    private:
        int offsetDelta_;
    };

    class SameFrame : public StackMapFrame
    {
    public:
        using StackMapFrame::StackMapFrame;
        [[nodiscard]] int getFrameType() const noexcept override
        {
            return SAME_FRAME_MIN + offsetDelta();
        }
    };

    class SameLocalsOneStackItem : public StackMapFrame
    {
    public:
        SameLocalsOneStackItem(int offsetDelta, std::unique_ptr<TypeInfo> stack)
            : StackMapFrame(offsetDelta)
            , stack_(std::move(stack))
        {}

        [[nodiscard]] int getFrameType() const noexcept override
        {
            return SAME_LOCALS_ONE_STACK_ITEM_MIN + offsetDelta();
        }
        [[nodiscard]] int getLength() const noexcept override { return 1 + stack_->getLength(); }
        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

        [[nodiscard]] const TypeInfo* stack() const noexcept { return stack_.get(); }
        void setStack(std::unique_ptr<TypeInfo> stack) noexcept { stack_ = std::move(stack); }

    private:
        std::unique_ptr<TypeInfo> stack_;
    };

    class SameLocalsOneStackItemExtended : public StackMapFrame
    {
    public:
        SameLocalsOneStackItemExtended(int offsetDelta, std::unique_ptr<TypeInfo> stack)
            : StackMapFrame(offsetDelta)
            , stack_(std::move(stack))
        {}

        [[nodiscard]] int getFrameType() const noexcept override
        {
            return SAME_LOCALS_ONE_STACK_ITEM_EXTENDED_MIN;
        }
        [[nodiscard]] int getLength() const noexcept override
        {
            return 1 + 2 + stack_->getLength();
        }
        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

        [[nodiscard]] const TypeInfo* stack() const noexcept { return stack_.get(); }
        void setStack(std::unique_ptr<TypeInfo> stack) noexcept { stack_ = std::move(stack); }

    private:
        std::unique_ptr<TypeInfo> stack_;
    };

    class ChopFrame : public StackMapFrame
    {
    public:
        ChopFrame(int offsetDelta, int absentVariables) noexcept
            : StackMapFrame(offsetDelta)
            , absentVariables_(absentVariables)
        {}

        [[nodiscard]] int getFrameType() const noexcept override
        {
            return CHOP_FRAME_MAX - absentVariables_ + 1;
        }
        [[nodiscard]] int getLength() const noexcept override { return 1 + 2; }

        [[nodiscard]] int absentVariables() const noexcept { return absentVariables_; }
        void              setAbsentVariables(int absentVariables) noexcept
        {
            absentVariables_ = absentVariables;
        }

    private:
        int absentVariables_;
    };

    class SameFrameExtended : public StackMapFrame
    {
    public:
        using StackMapFrame::StackMapFrame;
        [[nodiscard]] int getFrameType() const noexcept override { return SAME_FRAME_EXTENDED_MIN; }
        [[nodiscard]] int getLength() const noexcept override { return 1 + 2; }
    };

    class AppendFrame : public StackMapFrame
    {
    public:
        AppendFrame(int offsetDelta, std::vector<std::unique_ptr<TypeInfo>> additionalLocals)
            : StackMapFrame(offsetDelta)
            , additionalLocals_(std::move(additionalLocals))
        {}

        [[nodiscard]] int getFrameType() const noexcept override
        {
            return static_cast<int>(additionalLocals_.size()) + APPEND_FRAME_MIN - 1;
        }
        [[nodiscard]] int                         getLength() const noexcept override;
        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

        [[nodiscard]] const std::vector<std::unique_ptr<TypeInfo>>&
        additionalLocals() const noexcept
        {
            return additionalLocals_;
        }
        void setAdditionalLocals(std::vector<std::unique_ptr<TypeInfo>> additionalLocals) noexcept
        {
            additionalLocals_ = std::move(additionalLocals);
        }

    private:
        std::vector<std::unique_ptr<TypeInfo>> additionalLocals_;
    };

    class FullFrame : public StackMapFrame
    {
    public:
        FullFrame(int offsetDelta, std::vector<std::unique_ptr<TypeInfo>> locals,
                  std::vector<std::unique_ptr<TypeInfo>> stack)
            : StackMapFrame(offsetDelta)
            , locals_(std::move(locals))
            , stack_(std::move(stack))
        {}

        [[nodiscard]] int getFrameType() const noexcept override { return FULL_FRAME_MIN; }
        [[nodiscard]] int getLength() const noexcept override;
        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

        [[nodiscard]] const std::vector<std::unique_ptr<TypeInfo>>& locals() const noexcept
        {
            return locals_;
        }
        void setLocals(std::vector<std::unique_ptr<TypeInfo>> locals) noexcept
        {
            locals_ = std::move(locals);
        }

        [[nodiscard]] const std::vector<std::unique_ptr<TypeInfo>>& stack() const noexcept
        {
            return stack_;
        }
        void setStack(std::vector<std::unique_ptr<TypeInfo>> stack) noexcept
        {
            stack_ = std::move(stack);
        }

    private:
        std::vector<std::unique_ptr<TypeInfo>> locals_;
        std::vector<std::unique_ptr<TypeInfo>> stack_;
    };

    StackMapTableAttribute(const CpUtf8* name, std::vector<std::unique_ptr<StackMapFrame>> frames)
        : Attribute(name)
        , frames_(std::move(frames))
    {}

    [[nodiscard]] const std::vector<std::unique_ptr<StackMapFrame>>& frames() const noexcept
    {
        return frames_;
    }
    void setFrames(std::vector<std::unique_ptr<StackMapFrame>> frames) noexcept
    {
        frames_ = std::move(frames);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override;

private:
    std::vector<std::unique_ptr<StackMapFrame>> frames_;
};

}   // namespace jclassfile::model
