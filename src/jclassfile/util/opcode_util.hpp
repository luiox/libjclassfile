// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/util/OpcodeUtil.java
// 状态：阶段 5a 实现
// 备注：
//   - Java 用 Class.getFields() 反射读 Opcodes + ReservedOpcodes 的 int 字段，按声明顺序填入
//     names[opcode] = fieldName.toLowerCase()。C++ 无运行时反射，手动在 .cpp 中按相同顺序
//     填入 202 + 40 = 242 个名字。
//   - 返回 nullptr 表示未知 opcode（与原版 null 行为一致）。
//   - 名字保持小写（与原版 .toLowerCase() 一致），下划线保留原样。

#pragma once

namespace jclassfile::util {

class OpcodeUtil
{
public:
    [[nodiscard]] static const char* getOpcodeName(int opcode) noexcept;

private:
    static constexpr int     kNamesSize = 256;
    static const char* const kNames_[kNamesSize];
};

}   // namespace jclassfile::util
