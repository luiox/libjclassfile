// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/ConstPool.java
// 状态：阶段 2 实现
// 备注：原版实现 List<CpEntry> 接口，C++ 简化为最小子集（add/get/remove/size/iterator）。
//       上游 c73b1808 补齐 add(int,·) / addAll(int,·)（保持传入集合顺序）。
//       backing 存 std::vector<std::unique_ptr<CpEntry>>，index 0 预置 ImplZero 哨兵。
//       wide entry（LONG/DOUBLE）后追加 ImplWidePadding 占位，get() 对 tag<=0 返回 nullptr。
//       CpIterator 跳过 padding，仅返回真实 entry。

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

class ConstPool
{
public:
    class CpIterator
    {
    public:
        using CpConsumer = std::function<void(int index, const CpEntry* entry)>;

        CpIterator(const ConstPool* pool, int cursor) noexcept;

        void                         forEach(CpConsumer consumer) const;
        [[nodiscard]] bool           hasNext() const noexcept;
        [[nodiscard]] const CpEntry* next();
        [[nodiscard]] bool           hasPrevious() const noexcept;
        [[nodiscard]] const CpEntry* previous();
        [[nodiscard]] int            nextIndex() const noexcept;
        [[nodiscard]] int            previousIndex() const noexcept;

    private:
        const ConstPool* pool_;
        int              cursor_;
        int              lastCursor_;
    };

    ConstPool();
    ConstPool(const ConstPool&)            = delete;
    ConstPool& operator=(const ConstPool&) = delete;
    ~ConstPool()                           = default;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool        isEmpty() const noexcept;
    [[nodiscard]] bool        contains(const CpEntry* cp) const;
    [[nodiscard]] int         indexOf(const CpEntry* cp) const;
    [[nodiscard]] int         lastIndexOf(const CpEntry* cp) const;

    // 取 entry；index < 1 或 tag <= 0（padding）返回 nullptr。
    [[nodiscard]] CpEntry* get(int index) const;

    // add：转交所有权，setIndex 并在 wide 时追加 padding。
    void add(std::unique_ptr<CpEntry> cp);

    // 指定位置插入（转交所有权）：wide entry 先插 padding 占位，落位 [entry, padding]，
    // 随后重排 index。index 合法域 [1, size]；0 号哨兵槽与 wide padding 槽不可插入，
    // 违规抛 std::out_of_range（对应原版 IndexOutOfBoundsException）。
    void add(int index, std::unique_ptr<CpEntry> cp);

    // 上游 c73b1808：从 index 起按传入集合顺序逐个插入，wide entry 占 2 个槽位。
    // 空集合返回 false；index 非法时抛 std::out_of_range。
    bool addAll(int index, std::vector<std::unique_ptr<CpEntry>> entries);

    // set：替换 index 处的 entry，转交所有权，调用方放弃对原 entry 的引用。
    //      若新 entry 为 wide，旧 wide padding 保留（不会移除）；调用方需自行保证 wide 配对。
    void set(int index, std::unique_ptr<CpEntry> cp);

    // remove：返回被移除的 entry（移出所有权），自动重排后续 index。
    [[nodiscard]] std::unique_ptr<CpEntry> remove(int index);
    bool                                   remove(const CpEntry* cp);

    void clear();

    [[nodiscard]] CpIterator iterator() const noexcept;
    [[nodiscard]] CpIterator listIterator(int index) const;

    // 静态工厂：wide 填充 entry（外部用于手工构造）。
    [[nodiscard]] static std::unique_ptr<CpEntry> wideFiller();

private:
    void fixIndices(int startingIndex);

    // 插入位置校验（对应原版 requireValidInsertion）：index 须在 [1, size] 且
    // 不落在 wide padding 槽内，违规抛 std::out_of_range。
    void requireValidInsertion(int index) const;

    // 槽位是否为哨兵/填充（对应原版 isFiller）：tag <= 0。
    [[nodiscard]] bool isFiller(int index) const;

    std::vector<std::unique_ptr<CpEntry>> backing_;
};

}   // namespace jclassfile::model
