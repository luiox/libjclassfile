// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/constant/Placeholders.java

#include "jclassfile/model/constant/placeholders.hpp"

#include <unordered_set>

#include "jclassfile/model/constant/const_ref.hpp"
#include "jclassfile/model/constant/cross_cp_referencing.hpp"

namespace jclassfile::model {

namespace {

// 已访问集合：防环 + 防同节点经多条路径重复展开（与上游 IdentityHashMap 语义一致）。
using VisitedSet = std::unordered_set<const CpEntry*>;

// 递归 DFS：placeholder 可能不在给定条目的直接引用上，而是隔一跳或多跳
// （上游 62af2ae6：旧实现只查一层直接引用，多跳引用链漏检）。
bool containsPlaceholderDeep(const CpEntry* entry, VisitedSet& visited) noexcept
{
    if (entry == nullptr) return false;
    if (!visited.insert(entry).second) return false;
    if (auto* ref = entry->asCrossReferencing()) {
        for (const CpEntry* referenced : ref->getReferences()) {
            if (Placeholders::isPlaceholder(referenced) ||
                containsPlaceholderDeep(referenced, visited))
            {
                return true;
            }
        }
    }
    return false;
}

}   // namespace

// Meyers singleton: thread-safe init（C++11 起），程序生命周期内不销毁（与 Java static final
// 等价）。 实例用 new 分配、unique_ptr 持有；外部仅暴露 const 指针。

const CpUtf8* Placeholders::utf8() noexcept
{
    static const auto* p = new CpUtf8("");
    return p;
}

const CpClass* Placeholders::klass() noexcept
{
    static const auto* p = new CpClass(utf8());
    return p;
}

const CpNameType* Placeholders::nameType() noexcept
{
    static const auto* p = new CpNameType(utf8(), utf8());
    return p;
}

const ConstRefInternal* Placeholders::constRef() noexcept
{
    static const auto* p = new ConstRefInternal(klass(), nameType());
    return p;
}

bool Placeholders::isPlaceholder(const CpEntry* entry) noexcept
{
    if (entry == nullptr) return false;
    return entry == utf8() || entry == klass() || entry == nameType() || entry == constRef();
}

bool Placeholders::containsPlaceholder(const CpEntry* entry) noexcept
{
    if (entry == nullptr) return false;
    VisitedSet visited;
    return containsPlaceholderDeep(entry, visited);
}

bool Placeholders::isOrContainsPlaceholder(const CpEntry* entry) noexcept
{
    return isPlaceholder(entry) || containsPlaceholder(entry);
}

}   // namespace jclassfile::model
