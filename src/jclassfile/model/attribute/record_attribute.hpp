// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/RecordAttribute.java

#pragma once

#include <vector>

#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/behavior/attribute_holder.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class RecordAttribute : public Attribute
{
public:
    class RecordComponent : public behavior::CpAccessor, public AttributeHolder
    {
    public:
        RecordComponent(const CpUtf8* name, const CpUtf8* desc,
                        std::vector<const Attribute*> attributes) noexcept
            : name_(name)
            , desc_(desc)
            , attributes_(std::move(attributes))
        {}

        [[nodiscard]] const CpUtf8* name() const noexcept { return name_; }
        void                        setName(const CpUtf8* name) noexcept { name_ = name; }

        [[nodiscard]] const CpUtf8* desc() const noexcept { return desc_; }
        void                        setDesc(const CpUtf8* desc) noexcept { desc_ = desc; }

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
            return jclassfile::io::AttributeHolderType::RECORD_COMPONENT;
        }

        [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

        [[nodiscard]] int length() const noexcept;

    private:
        const CpUtf8*                 name_;
        const CpUtf8*                 desc_;
        std::vector<const Attribute*> attributes_;
    };

    RecordAttribute(const CpUtf8* name, std::vector<RecordComponent> components)
        : Attribute(name)
        , components_(std::move(components))
    {}

    [[nodiscard]] const std::vector<RecordComponent>& components() const noexcept
    {
        return components_;
    }
    void setComponents(std::vector<RecordComponent> components) noexcept
    {
        components_ = std::move(components);
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] int                         computeInternalLength() const noexcept override;

private:
    std::vector<RecordComponent> components_;
};

}   // namespace jclassfile::model
