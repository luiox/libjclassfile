// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/ConstPool.java

#include "jclassfile/model/const_pool.hpp"

#include <stdexcept>
#include <string>
#include <utility>

#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_internal.hpp"

namespace jclassfile::model {

namespace {

// ImplZero / ImplWidePadding：tag = INTERNAL (= 0) 的哨兵 entry。
// 直接继承 CpEntry，tag 设为 INTERNAL 让 ConstPool 按 tag<=0 跳过。
// （与 Java ImplZero/ImplWidePadding extends CpInternal 同构；C++ 为避免
//   diamond 而采用直接继承 + 共享 tag 值的简化形式。）
class ImplZero : public CpEntry
{
public:
    explicit ImplZero(CpTag tag)
        : CpEntry(tag)
    {}
    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override { return {this}; }
    [[nodiscard]] bool equals(const CpEntry& /*other*/) const override { return false; }
};

class ImplWidePadding : public CpEntry
{
public:
    explicit ImplWidePadding(CpTag tag)
        : CpEntry(tag)
    {}
    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override { return {this}; }
    [[nodiscard]] bool equals(const CpEntry& /*other*/) const override { return false; }
};

}   // namespace

// ---------- ConstPool ----------

ConstPool::ConstPool()
{
    auto zero = std::make_unique<ImplZero>(CpTag::INTERNAL);
    zero->setIndex(0);
    backing_.push_back(std::move(zero));
}

std::size_t ConstPool::size() const noexcept
{
    return backing_.size();
}

bool ConstPool::isEmpty() const noexcept
{
    return backing_.size() <= 1;
}

bool ConstPool::contains(const CpEntry* cp) const
{
    return indexOf(cp) >= 1;
}

int ConstPool::indexOf(const CpEntry* cp) const
{
    if (cp == nullptr) return -1;
    for (std::size_t i = 1; i < backing_.size(); ++i) {
        if (backing_[i].get() == cp) return static_cast<int>(i);
    }
    return -1;
}

int ConstPool::lastIndexOf(const CpEntry* cp) const
{
    if (cp == nullptr) return -1;
    for (std::size_t i = backing_.size(); i-- > 1;) {
        if (backing_[i].get() == cp) return static_cast<int>(i);
    }
    return -1;
}

CpEntry* ConstPool::get(int index) const
{
    if (index < 1 || static_cast<std::size_t>(index) >= backing_.size()) return nullptr;
    CpEntry* cp = backing_[index].get();
    if (static_cast<int>(cp->tag()) <= 0) return nullptr;
    return cp;
}

void ConstPool::add(std::unique_ptr<CpEntry> cp)
{
    if (!cp) return;
    int index = static_cast<int>(backing_.size());
    cp->setIndex(static_cast<ca::u16>(index));
    bool wide = cp->isWide();
    backing_.push_back(std::move(cp));
    if (wide) {
        auto pad = std::make_unique<ImplWidePadding>(CpTag::INTERNAL);
        pad->setIndex(static_cast<ca::u16>(backing_.size()));
        backing_.push_back(std::move(pad));
    }
}

void ConstPool::add(int index, std::unique_ptr<CpEntry> cp)
{
    if (!cp) return;
    requireValidInsertion(index);

    // wide entry 先插 padding 占位，使 wide 常量落位为 [entry, padding]。
    if (cp->isWide()) {
        auto pad = std::make_unique<ImplWidePadding>(CpTag::INTERNAL);
        backing_.insert(backing_.begin() + index, std::move(pad));
    }

    // 插入 entry 并重排其后的全部 index。
    backing_.insert(backing_.begin() + index, std::move(cp));
    fixIndices(index);
}

bool ConstPool::addAll(int index, std::vector<std::unique_ptr<CpEntry>> entries)
{
    // 上游 c73b1808：逐个右移插入点（wide entry 占 2 槽），保证集合顺序与
    // 传入顺序一致；旧实现固定从 index 插入，wide entry 会让后续元素错位。
    requireValidInsertion(index);
    if (entries.empty()) return false;

    int offset = 0;
    for (auto& cp : entries) {
        if (!cp) continue;
        bool wide = cp->isWide();
        add(index + offset, std::move(cp));
        offset += wide ? 2 : 1;
    }
    return true;
}

void ConstPool::set(int index, std::unique_ptr<CpEntry> cp)
{
    if (index < 1 || index >= static_cast<int>(backing_.size())) return;
    if (!cp) return;
    cp->setIndex(static_cast<ca::u16>(index));
    backing_[index] = std::move(cp);
}

std::unique_ptr<CpEntry> ConstPool::remove(int index)
{
    if (index < 1 || static_cast<std::size_t>(index) >= backing_.size()) return nullptr;
    auto removed = std::move(backing_[index]);
    bool wide    = removed->isWide();
    backing_.erase(backing_.begin() + index);
    if (wide && static_cast<std::size_t>(index) < backing_.size() &&
        static_cast<int>(backing_[index]->tag()) <= 0) {
        backing_.erase(backing_.begin() + index);
    }
    fixIndices(index);
    return removed;
}

bool ConstPool::remove(const CpEntry* cp)
{
    int i = indexOf(cp);
    if (i < 1) return false;
    (void)remove(i);
    return true;
}

void ConstPool::clear()
{
    backing_.clear();
    auto zero = std::make_unique<ImplZero>(CpTag::INTERNAL);
    zero->setIndex(0);
    backing_.push_back(std::move(zero));
}

ConstPool::CpIterator ConstPool::iterator() const noexcept
{
    return listIterator(1);
}

ConstPool::CpIterator ConstPool::listIterator(int index) const
{
    return CpIterator(this, index);
}

std::unique_ptr<CpEntry> ConstPool::wideFiller()
{
    return std::make_unique<ImplWidePadding>(CpTag::INTERNAL);
}

void ConstPool::fixIndices(int startingIndex)
{
    for (std::size_t i = static_cast<std::size_t>(startingIndex); i < backing_.size(); ++i) {
        backing_[i]->setIndex(static_cast<ca::u16>(i));
    }
}

void ConstPool::requireValidInsertion(int index) const
{
    if (index < 1 || index > static_cast<int>(backing_.size())) {
        throw std::out_of_range("Constant insertion index out of range: " + std::to_string(index));
    }
    if (index < static_cast<int>(backing_.size()) && isFiller(index)) {
        throw std::out_of_range("Cannot insert into a wide padding slot: " + std::to_string(index));
    }
}

bool ConstPool::isFiller(int index) const
{
    return static_cast<int>(backing_[static_cast<std::size_t>(index)]->tag()) <= 0;
}

// ---------- CpIterator ----------

ConstPool::CpIterator::CpIterator(const ConstPool* pool, int cursor) noexcept
    : pool_(pool)
    , cursor_(cursor)
    , lastCursor_(cursor)
{}

void ConstPool::CpIterator::forEach(CpConsumer consumer) const
{
    CpIterator it = *this;
    while (it.hasNext()) {
        int            i = it.cursor_;
        const CpEntry* e = it.next();
        if (consumer) consumer(i, e);
    }
}

bool ConstPool::CpIterator::hasNext() const noexcept
{
    return static_cast<std::size_t>(cursor_) < pool_->size();
}

const CpEntry* ConstPool::CpIterator::next()
{
    if (!hasNext()) return nullptr;
    CpEntry* cp = pool_->get(cursor_);
    if (cp == nullptr) return nullptr;
    lastCursor_ = cursor_;
    cursor_ += cp->isWide() ? 2 : 1;
    return cp;
}

bool ConstPool::CpIterator::hasPrevious() const noexcept
{
    return cursor_ > 1;
}

const CpEntry* ConstPool::CpIterator::previous()
{
    if (!hasPrevious()) return nullptr;
    CpEntry* cp = pool_->get(cursor_ - 1);
    if (cp == nullptr) return nullptr;
    if (static_cast<int>(cp->tag()) <= 0) {
        cp = pool_->get(cursor_ - 2);
    }
    if (cp == nullptr) return nullptr;
    lastCursor_ = cursor_;
    cursor_ -= cp->isWide() ? 2 : 1;
    return cp;
}

int ConstPool::CpIterator::nextIndex() const noexcept
{
    std::size_t s = pool_->size();
    int         n = cursor_ + 1;
    return n > static_cast<int>(s) ? static_cast<int>(s) : n;
}

int ConstPool::CpIterator::previousIndex() const noexcept
{
    if (!hasPrevious()) return -1;
    CpEntry* cp = pool_->get(cursor_ - 1);
    if (cp == nullptr) return -1;
    return static_cast<int>(cp->tag()) <= 0 ? cursor_ - 2 : cursor_ - 1;
}

}   // namespace jclassfile::model
