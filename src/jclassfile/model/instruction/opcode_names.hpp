// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/instruction/OpcodeNames.java

#pragma once

#include <string_view>

#include "opcodes.hpp"

namespace jclassfile::model {

class OpcodeNames
{
public:
    [[nodiscard]] static std::string_view name(int opcode) noexcept;
};

}   // namespace jclassfile::model
