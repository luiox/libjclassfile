#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "jstr/mutf8_string.hpp"
#include "libca/core/datatype.hpp"

namespace jclassfile::io::detail {

inline bool isContinuationByte(ca::u8 byte)
{
    return (byte & 0xC0u) == 0x80u;
}

inline void appendUtf8Unit(std::string& out, ca::u16 unit)
{
    if (unit <= 0x7Fu) {
        out.push_back(static_cast<char>(unit));
    }
    else if (unit <= 0x7FFu) {
        out.push_back(static_cast<char>(0xC0u | (unit >> 6)));
        out.push_back(static_cast<char>(0x80u | (unit & 0x3Fu)));
    }
    else {
        out.push_back(static_cast<char>(0xE0u | (unit >> 12)));
        out.push_back(static_cast<char>(0x80u | ((unit >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (unit & 0x3Fu)));
    }
}

inline void appendUtf8CodePoint(std::string& out, ca::u32 cp)
{
    if (cp <= 0x7Fu) {
        out.push_back(static_cast<char>(cp));
    }
    else if (cp <= 0x7FFu) {
        out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    }
    else if (cp <= 0xFFFFu) {
        out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    }
    else {
        out.push_back(static_cast<char>(0xF0u | (cp >> 18)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    }
}

// CAFED00D/Java compatibility: preserve lone surrogate units as their raw
// three-byte UTF-8 form so obfuscated crasher samples can still be rewritten.
inline std::string decodeMutf8LikeJava(jstr::MUtf8StringRef view)
{
    const auto*       data = reinterpret_cast<const ca::u8*>(view.data());
    const std::size_t size = view.size();

    std::vector<ca::u16> units;
    units.reserve(size);
    for (std::size_t off = 0; off < size;) {
        ca::u8 b0 = data[off];
        if ((b0 & 0x80u) == 0) {
            units.push_back(b0);
            off += 1;
            continue;
        }
        if ((b0 & 0xE0u) == 0xC0u) {
            if (off + 1 >= size || !isContinuationByte(data[off + 1])) {
                throw std::runtime_error("Truncated modified UTF-8 sequence");
            }
            ca::u16 value = static_cast<ca::u16>(((b0 & 0x1Fu) << 6) | (data[off + 1] & 0x3Fu));
            units.push_back(value);
            off += 2;
            continue;
        }
        if ((b0 & 0xF0u) == 0xE0u) {
            if (off + 2 >= size || !isContinuationByte(data[off + 1]) ||
                !isContinuationByte(data[off + 2])) {
                throw std::runtime_error("Truncated modified UTF-8 sequence");
            }
            ca::u16 value = static_cast<ca::u16>(
                ((b0 & 0x0Fu) << 12) | ((data[off + 1] & 0x3Fu) << 6) | (data[off + 2] & 0x3Fu));
            units.push_back(value);
            off += 3;
            continue;
        }
        throw std::runtime_error("Invalid modified UTF-8 sequence");
    }

    std::string out;
    out.reserve(size);
    for (std::size_t i = 0; i < units.size(); ++i) {
        ca::u16 unit = units[i];
        if (unit >= 0xD800u && unit <= 0xDBFFu && i + 1 < units.size()) {
            ca::u16 low = units[i + 1];
            if (low >= 0xDC00u && low <= 0xDFFFu) {
                ca::u32 cp = 0x10000u + (((unit - 0xD800u) << 10) | (low - 0xDC00u));
                appendUtf8CodePoint(out, cp);
                ++i;
                continue;
            }
        }
        appendUtf8Unit(out, unit);
    }
    return out;
}

}   // namespace jclassfile::io::detail
