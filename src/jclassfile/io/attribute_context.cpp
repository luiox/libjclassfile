// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/AttributeContext.java

#include "attribute_context.hpp"

namespace jclassfile::io {

const char* AttributeContext::name() const noexcept
{
    switch (type_) {
    case AttributeHolderType::CLASS: return "CLASS";
    case AttributeHolderType::FIELD: return "FIELD";
    case AttributeHolderType::METHOD: return "METHOD";
    case AttributeHolderType::RECORD_COMPONENT: return "RECORD_COMPONENT";
    case AttributeHolderType::ATTRIBUTE: return "ATTRIBUTE";
    }
    return "<unknown>";
}

}   // namespace jclassfile::io
