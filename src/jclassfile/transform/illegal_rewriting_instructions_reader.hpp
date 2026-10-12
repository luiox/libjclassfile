// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/transform/IllegalRewritingInstructionsReader.java

#pragma once

#include <memory>
#include <vector>

#include "jclassfile/io/fallback_instruction_reader.hpp"
#include "jclassfile/io/indexable_byte_stream.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/instruction/instruction.hpp"

namespace jclassfile::transform {

class IllegalRewritingInstructionsReader : public io::FallbackInstructionReader
{
public:
    IllegalRewritingInstructionsReader(const model::ConstPool& cp, int classVersion)
        : cp_(cp)
        , classVersion_(classVersion)
    {}

    [[nodiscard]] std::vector<std::unique_ptr<model::Instruction>> read(
        int opcode, io::IndexableByteStream& is) override;

    [[nodiscard]] bool wasRewritten() const noexcept { return rewritten_; }

private:
    const model::ConstPool& cp_;
    int                     classVersion_;
    bool                    rewritten_ = false;
};

}   // namespace jclassfile::transform
