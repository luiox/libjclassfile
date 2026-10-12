// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/InnerClassesAttribute.java

#pragma once

#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class InnerClassesAttribute : public Attribute
{
public:
    class InnerClass : public behavior::CpAccessor
    {
    public:
        InnerClass(const CpClass* innerClassInfo, const CpClass* outerClassInfo,
                   const CpUtf8* innerName, int innerClassAccessFlags) noexcept
            : innerClassInfo_(innerClassInfo)
            , outerClassInfo_(outerClassInfo)
            , innerName_(innerName)
            , innerClassAccessFlags_(innerClassAccessFlags)
        {}

        [[nodiscard]] const CpClass* innerClassInfo() const noexcept { return innerClassInfo_; }
        void                         setInnerClassInfo(const CpClass* innerClassInfo) noexcept
        {
            innerClassInfo_ = innerClassInfo;
        }

        [[nodiscard]] const CpClass* outerClassInfo() const noexcept { return outerClassInfo_; }
        void                         setOuterClassInfo(const CpClass* outerClassInfo) noexcept
        {
            outerClassInfo_ = outerClassInfo;
        }

        [[nodiscard]] const CpUtf8* innerName() const noexcept { return innerName_; }
        void setInnerName(const CpUtf8* innerName) noexcept { innerName_ = innerName; }

        [[nodiscard]] int innerClassAccessFlags() const noexcept { return innerClassAccessFlags_; }
        void              setInnerClassAccessFlags(int innerClassAccessFlags) noexcept
        {
            innerClassAccessFlags_ = innerClassAccessFlags;
        }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    private:
        const CpClass* innerClassInfo_;
        const CpClass* outerClassInfo_;
        const CpUtf8*  innerName_;
        int            innerClassAccessFlags_;
    };

    InnerClassesAttribute(const CpUtf8* name, std::vector<InnerClass> classes)
        : Attribute(name)
        , innerClasses_(std::move(classes))
    {}

    [[nodiscard]] const std::vector<InnerClass>& innerClasses() const noexcept
    {
        return innerClasses_;
    }
    void setInnerClasses(std::vector<InnerClass> innerClasses) noexcept
    {
        innerClasses_ = std::move(innerClasses);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override
    {
        return 2 + static_cast<int>(innerClasses_.size()) * 8;
    }

private:
    std::vector<InnerClass> innerClasses_;
};

}   // namespace jclassfile::model
