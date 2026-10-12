// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/CpUtf8.java
// 状态：阶段 2 实现

#pragma once

#include <string>
#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class CpUtf8 : public CpEntry
{
public:
    explicit CpUtf8(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    void                             setText(std::string text);

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] bool                        equals(const CpEntry& other) const override;

private:
    std::string text_;
};

}   // namespace jclassfile::model
