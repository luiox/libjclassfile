// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/Modifiers.java

#pragma once

#include <cstdint>

namespace jclassfile::model {

namespace Modifiers {

// Applies to: class, field, method
inline constexpr uint16_t ACC_PUBLIC    = 0x0001;
inline constexpr uint16_t ACC_PRIVATE   = 0x0002;
inline constexpr uint16_t ACC_PROTECTED = 0x0004;
// Applies to: field, method
inline constexpr uint16_t ACC_STATIC = 0x0008;
// Applies to: class, field, method, parameter
inline constexpr uint16_t ACC_FINAL = 0x0010;
// Applies to: class
inline constexpr uint16_t ACC_SUPER = 0x0020;
// Applies to: method (same value as ACC_SUPER)
inline constexpr uint16_t ACC_SYNCHRONIZED = 0x0020;
// Applies to: module (same value as ACC_SUPER)
inline constexpr uint16_t ACC_OPEN = 0x0020;
// Applies to: module requires (same value as ACC_SUPER)
inline constexpr uint16_t ACC_TRANSITIVE = 0x0020;
// Applies to: field
inline constexpr uint16_t ACC_VOLATILE = 0x0040;
// Applies to: method (same value as ACC_VOLATILE)
inline constexpr uint16_t ACC_BRIDGE = 0x0040;
// Applies to: module requires (same value as ACC_VOLATILE)
inline constexpr uint16_t ACC_STATIC_PHASE = 0x0040;
// Applies to: method
inline constexpr uint16_t ACC_VARARGS = 0x0080;
// Applies to: field (same value as ACC_VARARGS)
inline constexpr uint16_t ACC_TRANSIENT = 0x0080;
// Applies to: method
inline constexpr uint16_t ACC_NATIVE = 0x0100;
// Applies to: class
inline constexpr uint16_t ACC_INTERFACE = 0x0200;
// Applies to: class, method
inline constexpr uint16_t ACC_ABSTRACT = 0x0400;
// Applies to: method
inline constexpr uint16_t ACC_STRICT = 0x0800;
// Applies to: class, field, method, parameter, module
inline constexpr uint16_t ACC_SYNTHETIC = 0x1000;
// Applies to: class
inline constexpr uint16_t ACC_ANNOTATION = 0x2000;
// Applies to: class, field, inner class
inline constexpr uint16_t ACC_ENUM = 0x4000;
// Applies to: field, method, parameter, module
inline constexpr uint16_t ACC_MANDATED = 0x8000;
// Applies to: class (same value as ACC_MANDATED)
inline constexpr uint16_t ACC_MODULE = 0x8000;

inline constexpr bool has(uint16_t mask, uint16_t flag) noexcept
{
    return (mask & flag) != 0;
}

inline constexpr uint16_t flip(uint16_t mask, uint16_t flag) noexcept
{
    return has(mask, flag) ? static_cast<uint16_t>(mask & ~flag)
                           : static_cast<uint16_t>(mask | flag);
}

inline constexpr uint16_t remove(uint16_t mask, uint16_t flag) noexcept
{
    return static_cast<uint16_t>(mask & ~flag);
}

inline bool hasAll(uint16_t mask, std::initializer_list<uint16_t> flags) noexcept
{
    for (uint16_t flag : flags)
        if ((mask & flag) == 0) return false;
    return true;
}

inline bool hasAny(uint16_t mask, std::initializer_list<uint16_t> flags) noexcept
{
    for (uint16_t flag : flags)
        if ((mask & flag) != 0) return true;
    return false;
}

inline uint16_t createMask(std::initializer_list<uint16_t> flags) noexcept
{
    uint16_t acc = 0;
    for (uint16_t flag : flags) acc |= flag;
    return acc;
}

}   // namespace Modifiers

}   // namespace jclassfile::model
