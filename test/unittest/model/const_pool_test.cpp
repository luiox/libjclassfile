// 阶段 2：constant pool 模型 + Placeholders + ConstPool 基础行为测试。
// 覆盖：CpUtf8 / CpClass 构造与 equals；CpLong wide 行为；
//       Placeholders 4 个单例身份（pointer identity）；
//       ConstPool add/get/remove + 宽条目 padding 行为；
//       上游 c73b1808：ConstPool addAll 保持传入集合顺序。

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"
#include "jclassfile/model/constant/const_ref.hpp"
#include "jclassfile/model/constant/const_ref_internal.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_double.hpp"
#include "jclassfile/model/constant/cp_dynamic.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_field_ref.hpp"
#include "jclassfile/model/constant/cp_invoke_dynamic.hpp"
#include "jclassfile/model/constant/cp_int.hpp"
#include "jclassfile/model/constant/cp_long.hpp"
#include "jclassfile/model/constant/cp_method_handle.hpp"
#include "jclassfile/model/constant/cp_method_ref.hpp"
#include "jclassfile/model/constant/cp_method_type.hpp"
#include "jclassfile/model/constant/cp_module.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
#include "jclassfile/model/constant/cp_package.hpp"
#include "jclassfile/model/constant/cp_string.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/constant/cross_cp_referencing.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"
#include "jclassfile/model/constant/placeholders.hpp"

using namespace jclassfile::model;

// ---------- CpEntry 子类基础行为 ----------

TEST(CpEntryTagTest, Utf8TagIsOne)
{
    CpUtf8 u("hello");
    EXPECT_EQ(u.tag(), CpTag::UTF8);
    EXPECT_FALSE(u.isWide());
}

TEST(CpEntryTagTest, LongAndDoubleAreWide)
{
    CpLong   l(static_cast<ca::i64>(42));
    CpDouble d(3.14);
    EXPECT_EQ(l.tag(), CpTag::LONG);
    EXPECT_EQ(d.tag(), CpTag::DOUBLE);
    EXPECT_TRUE(l.isWide());
    EXPECT_TRUE(d.isWide());
}

TEST(CpEntryTagTest, ClassStringAndRefAreNotWide)
{
    CpUtf8   u("Foo");
    CpClass  c(&u);
    CpString s(&u);
    EXPECT_FALSE(c.isWide());
    EXPECT_FALSE(s.isWide());
}

// ---------- LoadableConstant marker ----------

TEST(LoadableMarkerTest, ClassStringAndDynamicAreLoadable)
{
    CpUtf8     u("x");
    CpClass    c(&u);
    CpString   s(&u);
    CpUtf8     n("name"), t("(I)V");
    CpNameType nt(&n, &t);
    CpDynamic  dy(0, &nt);
    EXPECT_TRUE(c.isLoadableConstant());
    EXPECT_TRUE(s.isLoadableConstant());
    EXPECT_TRUE(dy.isLoadableConstant());
}

TEST(LoadableMarkerTest, Utf8NameTypeAndFieldRefAreNotLoadable)
{
    CpUtf8     u("x"), n("name"), t("(I)V");
    CpClass    c(&u);
    CpNameType nt(&n, &t);
    CpFieldRef fr(&c, &nt);
    EXPECT_FALSE(u.isLoadableConstant());
    EXPECT_FALSE(nt.isLoadableConstant());
    EXPECT_FALSE(fr.isLoadableConstant());
}

// ---------- CrossCpReferencing ----------

TEST(CrossCpRefTest, FieldRefReferencesClassAndNameType)
{
    CpUtf8     u("Foo"), n("x"), t("I");
    CpNameType nt(&n, &t);
    CpClass    c(&u);
    CpFieldRef fr(&c, &nt);

    auto* ref = fr.asCrossReferencing();
    ASSERT_NE(ref, nullptr);
    auto refs = ref->getReferences();
    ASSERT_EQ(refs.size(), 2u);
    EXPECT_EQ(refs[0], &c);
    EXPECT_EQ(refs[1], &nt);
}

TEST(CrossCpRefTest, Utf8IsNotCrossReferencing)
{
    CpUtf8 u("hello");
    EXPECT_FALSE(u.isCrossReferencing());
    EXPECT_EQ(u.asCrossReferencing(), nullptr);
}

// ---------- equals 语义（dynamic_cast 严格同类型）----------

TEST(EqualsTest, SameTextUtf8Equal)
{
    CpUtf8 a("hello");
    CpUtf8 b("hello");
    CpUtf8 c("world");
    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
}

TEST(EqualsTest, DifferentTypesNotEqual)
{
    CpUtf8  u("hello");
    CpClass c(&u);
    EXPECT_FALSE(c.equals(u));
    EXPECT_FALSE(u.equals(c));
}

TEST(EqualsTest, ConstRefSubclassEqualsUsesBaseFields)
{
    CpUtf8     u("Foo"), n("bar"), t("(I)V");
    CpNameType nt1(&n, &t), nt2(&n, &t);
    CpClass    c1(&u), c2(&u);
    CpFieldRef fr1(&c1, &nt1);
    CpFieldRef fr2(&c2, &nt2);
    // 同 classRef + 同 nameType → 相等
    EXPECT_TRUE(fr1.equals(fr2));

    CpUtf8     n3("baz");
    CpNameType nt3(&n3, &t);
    CpFieldRef fr3(&c1, &nt3);
    EXPECT_FALSE(fr1.equals(fr3));
}

// ---------- Placeholders ----------

TEST(PlaceholdersTest, SameInstanceReturned)
{
    const CpUtf8* u1 = Placeholders::utf8();
    const CpUtf8* u2 = Placeholders::utf8();
    EXPECT_EQ(u1, u2);

    const CpClass* c1 = Placeholders::klass();
    const CpClass* c2 = Placeholders::klass();
    EXPECT_EQ(c1, c2);
}

TEST(PlaceholdersTest, IsPlaceholderMatchesFour)
{
    EXPECT_TRUE(Placeholders::isPlaceholder(Placeholders::utf8()));
    EXPECT_TRUE(Placeholders::isPlaceholder(Placeholders::klass()));
    EXPECT_TRUE(Placeholders::isPlaceholder(Placeholders::nameType()));
    EXPECT_TRUE(Placeholders::isPlaceholder(Placeholders::constRef()));
    EXPECT_FALSE(Placeholders::isPlaceholder(nullptr));

    CpUtf8 real("real");
    EXPECT_FALSE(Placeholders::isPlaceholder(&real));
}

TEST(PlaceholdersTest, ContainsPlaceholderDetectsReferences)
{
    CpUtf8     real("real"), n("name"), t("(I)V");
    CpNameType realNt(&n, &t);
    CpClass    realClass(&real);
    CpFieldRef realRef(&realClass, &realNt);
    EXPECT_FALSE(Placeholders::containsPlaceholder(&realRef));
    EXPECT_FALSE(Placeholders::isOrContainsPlaceholder(&realRef));

    // FieldRef whose classRef is the CLASS placeholder
    CpFieldRef placeholderRef(Placeholders::klass(), &realNt);
    EXPECT_TRUE(Placeholders::containsPlaceholder(&placeholderRef));
    EXPECT_TRUE(Placeholders::isOrContainsPlaceholder(&placeholderRef));
}

TEST(PlaceholdersTest, ConstRefInternalIsPlaceholder)
{
    const ConstRefInternal* pri = Placeholders::constRef();
    EXPECT_TRUE(Placeholders::isPlaceholder(pri));
    // constRef 内部引用 CLASS/NAME_TYPE 哨兵，containsPlaceholder 也为 true
    EXPECT_TRUE(Placeholders::containsPlaceholder(pri));
}

// 上游 62af2ae6：placeholder 不在直接引用层、而是隔一跳或多跳时也要检出。
TEST(PlaceholdersTest, ContainsPlaceholderDetectsTransitiveReferences)
{
    // 两跳：MethodHandle → ConstRef → CLASS 占位符（旧实现只查一层直接引用，漏检）。
    CpUtf8           realN("x"), realT("(I)V"), realC("Foo");
    CpNameType       realNt(&realN, &realT);
    ConstRefInternal refToPlaceholder(Placeholders::klass(), &realNt);
    CpMethodHandle   mh(1, &refToPlaceholder);
    EXPECT_TRUE(Placeholders::containsPlaceholder(&mh));
    EXPECT_TRUE(Placeholders::isOrContainsPlaceholder(&mh));

    // 同构但全真实引用的链：不得误报。
    CpClass          realClass(&realC);
    ConstRefInternal realRef(&realClass, &realNt);
    CpMethodHandle   realMh(1, &realRef);
    EXPECT_FALSE(Placeholders::containsPlaceholder(&realMh));
    EXPECT_FALSE(Placeholders::isOrContainsPlaceholder(&realMh));

    // 一跳引用（dynamic → NAME_TYPE 占位符）依旧检出。
    CpDynamic dy(0, Placeholders::nameType());
    EXPECT_TRUE(Placeholders::containsPlaceholder(&dy));

    // 重复访问同一节点（菱形/环状引用图的防御路径）不应死循环。
    EXPECT_TRUE(Placeholders::containsPlaceholder(&mh));
}

// ---------- ConstPool ----------

TEST(ConstPoolTest, InitialSizeIsOne)
{
    ConstPool pool;
    EXPECT_EQ(pool.size(), 1u);
    EXPECT_TRUE(pool.isEmpty());
    EXPECT_EQ(pool.get(0), nullptr);
    EXPECT_EQ(pool.get(1), nullptr);
}

TEST(ConstPoolTest, AddUtf8AssignsIndex)
{
    ConstPool pool;
    auto      u = std::make_unique<CpUtf8>("hello");
    pool.add(std::move(u));
    EXPECT_EQ(pool.size(), 2u);
    CpEntry* got = pool.get(1);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->tag(), CpTag::UTF8);
    EXPECT_EQ(got->index(), 1u);
}

TEST(ConstPoolTest, LongAppendsPadding)
{
    ConstPool pool;
    pool.add(std::make_unique<CpLong>(static_cast<ca::i64>(123)));
    // 1 sentinel + 1 long + 1 padding = 3
    EXPECT_EQ(pool.size(), 3u);
    EXPECT_NE(pool.get(1), nullptr);
    EXPECT_EQ(pool.get(2), nullptr);   // padding → nullptr
}

TEST(ConstPoolTest, DoubleAppendsPadding)
{
    ConstPool pool;
    pool.add(std::make_unique<CpDouble>(2.5));
    EXPECT_EQ(pool.size(), 3u);
    EXPECT_NE(pool.get(1), nullptr);
    EXPECT_EQ(pool.get(2), nullptr);
}

TEST(ConstPoolTest, IteratorSkipsPadding)
{
    ConstPool pool;
    pool.add(std::make_unique<CpUtf8>("a"));
    pool.add(std::make_unique<CpLong>(static_cast<ca::i64>(1)));
    pool.add(std::make_unique<CpUtf8>("b"));

    std::vector<CpTag> seen;
    auto               collect = [&seen](int, const CpEntry* e) { seen.push_back(e->tag()); };
    pool.iterator().forEach(collect);
    ASSERT_EQ(seen.size(), 3u);
    EXPECT_EQ(seen[0], CpTag::UTF8);
    EXPECT_EQ(seen[1], CpTag::LONG);
    EXPECT_EQ(seen[2], CpTag::UTF8);
}

TEST(ConstPoolTest, IndexOfFindsEntry)
{
    ConstPool     pool;
    auto          u   = std::make_unique<CpUtf8>("hello");
    const CpUtf8* raw = u.get();
    pool.add(std::move(u));
    EXPECT_EQ(pool.indexOf(raw), 1);
    EXPECT_EQ(pool.lastIndexOf(raw), 1);
}

TEST(ConstPoolTest, RemoveShiftsIndices)
{
    ConstPool     pool;
    auto          a  = std::make_unique<CpUtf8>("a");
    auto          b  = std::make_unique<CpUtf8>("b");
    auto          c  = std::make_unique<CpUtf8>("c");
    const CpUtf8* ra = a.get();
    const CpUtf8* rb = b.get();
    const CpUtf8* rc = c.get();
    pool.add(std::move(a));
    pool.add(std::move(b));
    pool.add(std::move(c));

    ASSERT_TRUE(pool.remove(rb));
    EXPECT_EQ(pool.size(), 3u);
    EXPECT_EQ(ra->index(), 1u);
    EXPECT_EQ(rc->index(), 2u);
}

TEST(ConstPoolTest, ClearResetsToOneEntry)
{
    ConstPool pool;
    pool.add(std::make_unique<CpUtf8>("a"));
    pool.add(std::make_unique<CpUtf8>("b"));
    pool.clear();
    EXPECT_EQ(pool.size(), 1u);
    EXPECT_TRUE(pool.isEmpty());
}

TEST(ConstPoolTest, WideFillerProducesInternalEntry)
{
    auto filler = ConstPool::wideFiller();
    ASSERT_NE(filler, nullptr);
    EXPECT_TRUE(static_cast<int>(filler->tag()) <= 0);
}

// ---------- addAll（上游 c73b1808：保持传入集合顺序）----------

TEST(ConstPoolAddAllTest, IndexedAddAllPreservesOrder)
{
    ConstPool pool;
    auto          seed    = std::make_unique<CpUtf8>("seed");
    const CpUtf8* rawSeed = seed.get();
    pool.add(std::move(seed));

    auto          a       = std::make_unique<CpUtf8>("a");
    auto          longE   = std::make_unique<CpLong>(static_cast<ca::i64>(1));
    auto          b       = std::make_unique<CpUtf8>("b");
    const CpUtf8* rawA    = a.get();
    const CpLong* rawLong = longE.get();
    const CpUtf8* rawB    = b.get();

    std::vector<std::unique_ptr<CpEntry>> batch;
    batch.push_back(std::move(a));
    batch.push_back(std::move(longE));
    batch.push_back(std::move(b));

    // 插入点 2：backing 落位 [zero, seed, a, long, padding, b]，集合顺序保持。
    EXPECT_TRUE(pool.addAll(2, std::move(batch)));

    EXPECT_EQ(pool.size(), 6u);
    EXPECT_EQ(pool.get(1), rawSeed);
    EXPECT_EQ(pool.get(2), rawA);
    EXPECT_EQ(pool.get(3), rawLong);
    EXPECT_EQ(pool.get(4), nullptr);   // wide padding → nullptr
    EXPECT_EQ(pool.get(5), rawB);
    EXPECT_EQ(rawA->index(), 2u);
    EXPECT_EQ(rawLong->index(), 3u);
    EXPECT_EQ(rawB->index(), 5u);
}

TEST(ConstPoolAddAllTest, IndexedAddAllEmptyReturnsFalse)
{
    ConstPool pool;
    pool.add(std::make_unique<CpUtf8>("seed"));

    EXPECT_FALSE(pool.addAll(1, {}));
    EXPECT_EQ(pool.size(), 2u);
}

TEST(ConstPoolAddAllTest, IndexedAddAllRejectsZeroSlot)
{
    ConstPool pool;
    std::vector<std::unique_ptr<CpEntry>> batch;
    batch.push_back(std::make_unique<CpInt>(1));
    EXPECT_THROW(pool.addAll(0, std::move(batch)), std::out_of_range);
}
