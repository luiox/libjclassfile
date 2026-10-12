// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/Placeholders.java
// 状态：阶段 2 实现
// 备注：4 个静态单例（UTF8 / CLASS / NAME_TYPE / CONST_REF），用于 ClassFileReader
//       在构造 CpEntry 引用链时的"未知"占位。pointer identity 即 placeholder 身份。
//       原版 setter 抛 IllegalStateException；C++ 中 setIndex 非 virtual 无法拦截，
//       改为依赖"placeholder 不应被修改"的 API 契约（与 Java 等价的不变量）。

#pragma once

#include "jclassfile/model/constant/const_ref_internal.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

class Placeholders
{
public:
    [[nodiscard]] static const CpUtf8*           utf8() noexcept;
    [[nodiscard]] static const CpClass*          klass() noexcept;
    [[nodiscard]] static const CpNameType*       nameType() noexcept;
    [[nodiscard]] static const ConstRefInternal* constRef() noexcept;

    [[nodiscard]] static bool isPlaceholder(const CpEntry* entry) noexcept;
    [[nodiscard]] static bool containsPlaceholder(const CpEntry* entry) noexcept;
    [[nodiscard]] static bool isOrContainsPlaceholder(const CpEntry* entry) noexcept;
};

}   // namespace jclassfile::model
