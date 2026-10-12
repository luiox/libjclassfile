// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/CodeAttribute.java

#pragma once

#include <vector>

#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/attribute/code_utilities.hpp"
#include "jclassfile/model/behavior/attribute_holder.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/instruction/instruction.hpp"

namespace jclassfile::model {

class CodeAttribute : public Attribute, public CodeUtilities, public AttributeHolder
{
public:
    class ExceptionTableEntry : public behavior::CpAccessor
    {
    public:
        ExceptionTableEntry(int startPc, int endPc, int handlerPc,
                            const CpClass* catchType) noexcept
            : startPc_(startPc)
            , endPc_(endPc)
            , handlerPc_(handlerPc)
            , catchType_(catchType)
        {}

        [[nodiscard]] int startPc() const noexcept { return startPc_; }
        void              setStartPc(int startPc) noexcept { startPc_ = startPc; }

        [[nodiscard]] int endPc() const noexcept { return endPc_; }
        void              setEndPc(int endPc) noexcept { endPc_ = endPc; }

        [[nodiscard]] int handlerPc() const noexcept { return handlerPc_; }
        void              setHandlerPc(int handlerPc) noexcept { handlerPc_ = handlerPc; }

        [[nodiscard]] const CpClass* catchType() const noexcept { return catchType_; }
        void setCatchType(const CpClass* catchType) noexcept { catchType_ = catchType; }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    private:
        int            startPc_;
        int            endPc_;
        int            handlerPc_;
        const CpClass* catchType_;
    };

    CodeAttribute(const CpUtf8* name, int maxStack, int maxLocals,
                  std::vector<const Instruction*>  instructions,
                  std::vector<ExceptionTableEntry> exceptionTable,
                  std::vector<const Attribute*>    attributes) noexcept
        : Attribute(name)
        , exceptionTable_(std::move(exceptionTable))
        , attributes_(std::move(attributes))
        , instructions_(std::move(instructions))
        , maxStack_(maxStack)
        , maxLocals_(maxLocals)
    {}

    [[nodiscard]] const std::vector<const Instruction*>& instructions() const noexcept override
    {
        return instructions_;
    }
    void setInstructions(std::vector<const Instruction*> instructions) noexcept
    {
        instructions_ = std::move(instructions);
    }

    [[nodiscard]] int maxStack() const noexcept { return maxStack_; }
    void              setMaxStack(int maxStack) noexcept { maxStack_ = maxStack; }

    [[nodiscard]] int maxLocals() const noexcept { return maxLocals_; }
    void              setMaxLocals(int maxLocals) noexcept { maxLocals_ = maxLocals; }

    [[nodiscard]] const std::vector<ExceptionTableEntry>& exceptionTable() const noexcept
    {
        return exceptionTable_;
    }
    void setExceptionTable(std::vector<ExceptionTableEntry> exceptionTable) noexcept
    {
        exceptionTable_ = std::move(exceptionTable);
    }

    [[nodiscard]] std::vector<const Attribute*>& attributes() noexcept override
    {
        return attributes_;
    }
    [[nodiscard]] const std::vector<const Attribute*>& attributes() const noexcept override
    {
        return attributes_;
    }

    void setAttributes(std::vector<const Attribute*> attributes) noexcept override
    {
        attributes_ = std::move(attributes);
    }

    [[nodiscard]] jclassfile::io::AttributeHolderType holderType() const noexcept override
    {
        return jclassfile::io::AttributeHolderType::ATTRIBUTE;
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override;
    [[nodiscard]] AttrKind kind() const noexcept override { return AttrKind::Code; }
    [[nodiscard]] bool     isCodeAttribute() const noexcept override { return true; }

private:
    std::vector<ExceptionTableEntry> exceptionTable_;
    std::vector<const Attribute*>    attributes_;
    std::vector<const Instruction*>  instructions_;
    int                              maxStack_;
    int                              maxLocals_;
};

}   // namespace jclassfile::model
