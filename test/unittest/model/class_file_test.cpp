// 阶段 4a：ClassFile / ClassMember / Field / Method + InvalidClassException 基础行为测试。
// 覆盖：
//   - Accessible 接口（getAccess/setAccess）通过 ClassFile/Field/Method 多态调用；
//   - ClassFile 构造 / getter / setter / cpAccesses 行为（含 superClass=null）；
//   - ClassMember cpAccesses（name + type + attributes）；
//   - Method cpAccesses 在 ACC_NATIVE/ACC_ABSTRACT 上跳过 Code attribute 的 cpAccesses；
//   - Field/Method holderType 区分 + toString 格式；
//   - InvalidClassException 构造（String / Exception cause） + what() 文本。

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "jclassfile/invalid_class_exception.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/class_member.hpp"
#include "jclassfile/model/field.hpp"
#include "jclassfile/model/method.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/default_attribute.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/instruction/instruction.hpp"

using namespace jclassfile::model;
using jclassfile::io::AttributeHolderType;
using jclassfile::InvalidClassException;

namespace {

// 阶段 5 前的最小 Instruction stub。
class StubInsn : public Instruction
{
public:
    StubInsn(int op, int size)
        : Instruction(op)
        , op_(op)
        , size_(size)
    {}
    int                         getOpcode() const noexcept { return op_; }
    int                         computeSize() const noexcept override { return size_; }
    jclassfile::model::InsnKind kind() const noexcept override
    {
        return jclassfile::model::InsnKind::Basic;
    }

private:
    int op_;
    int size_;
};

// 构造一个最小可用的 Code attribute。CodeAttribute.placeholder isCodeAttribute() == true。
std::unique_ptr<CodeAttribute> makeCodeAttr(const CpUtf8* name)
{
    return std::make_unique<CodeAttribute>(name,
                                           /*maxStack*/ 1,
                                           /*maxLocals*/ 1,
                                           std::vector<const Instruction*>{},
                                           std::vector<CodeAttribute::ExceptionTableEntry>{},
                                           std::vector<const Attribute*>{});
}

}   // namespace

// ---------- Accessible 多态 ----------

TEST(AccessibleTest, GetSetAccessThroughBasePointer)
{
    CpUtf8      n("x"), t("I");
    Field       f({}, 0, &n, &t);
    Accessible* base = &f;
    EXPECT_EQ(base->getAccess(), 0);
    base->setAccess(0x0001);
    EXPECT_EQ(f.getAccess(), 0x0001);
}

TEST(AccessibleTest, ClassFileImplementsAccessible)
{
    CpUtf8      n("Foo"), t("I");
    CpClass     c(&n);
    auto        pool = std::make_unique<ConstPool>();
    ClassFile   cf(0, 52, std::move(pool), 0x0001, &c, nullptr, {}, {}, {}, {});
    Accessible* base = &cf;
    EXPECT_EQ(base->getAccess(), 0x0001);
    base->setAccess(0x0020);
    EXPECT_EQ(cf.getAccess(), 0x0020);
}

// ---------- ClassFile 基础行为 ----------

TEST(ClassFileTest, HolderTypeIsClass)
{
    CpUtf8    n("Foo"), t("I");
    CpClass   c(&n);
    ClassFile cf(0, 52, std::make_unique<ConstPool>(), 0, &c, nullptr, {}, {}, {}, {});
    EXPECT_EQ(cf.holderType(), AttributeHolderType::CLASS);
}

TEST(ClassFileTest, GetNameFromThisClass)
{
    CpUtf8    n("com/example/Foo"), t("I");
    CpClass   c(&n);
    ClassFile cf(0, 52, std::make_unique<ConstPool>(), 0, &c, nullptr, {}, {}, {}, {});
    EXPECT_EQ(cf.getName(), "com/example/Foo");
}

TEST(ClassFileTest, GetSuperNameNullForNullSuper)
{
    CpUtf8    n("Foo");
    CpClass   c(&n);
    ClassFile cf(0, 52, std::make_unique<ConstPool>(), 0, &c, nullptr, {}, {}, {}, {});
    EXPECT_EQ(cf.getSuperName(), "");
}

TEST(ClassFileTest, GetSuperNameWhenSet)
{
    CpUtf8    n("Foo"), p("java/lang/Object");
    CpClass   c(&n), s(&p);
    ClassFile cf(0, 52, std::make_unique<ConstPool>(), 0, &c, &s, {}, {}, {}, {});
    EXPECT_EQ(cf.getSuperName(), "java/lang/Object");
}

TEST(ClassFileTest, VersionGettersSetters)
{
    CpUtf8    n("X");
    CpClass   c(&n);
    ClassFile cf(0, 52, std::make_unique<ConstPool>(), 0, &c, nullptr, {}, {}, {}, {});
    EXPECT_EQ(cf.getVersionMinor(), 0);
    EXPECT_EQ(cf.getVersionMajor(), 52);
    cf.setVersionMinor(3);
    cf.setVersionMajor(61);
    EXPECT_EQ(cf.getVersionMinor(), 3);
    EXPECT_EQ(cf.getVersionMajor(), 61);
}

TEST(ClassFileTest, GetCpReturnsEntryFromPool)
{
    CpUtf8  n("Foo");
    CpClass c(&n);
    auto    pool = std::make_unique<ConstPool>();
    pool->add(std::make_unique<CpUtf8>("hello"));
    ClassFile cf(0, 52, std::move(pool), 0, &c, nullptr, {}, {}, {}, {});
    auto*     entry = cf.getCp(1);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->tag(), CpTag::UTF8);
}

TEST(ClassFileTest, GetCpReturnsNullForInvalidIndex)
{
    CpUtf8    n("X");
    CpClass   c(&n);
    ClassFile cf(0, 52, std::make_unique<ConstPool>(), 0, &c, nullptr, {}, {}, {}, {});
    EXPECT_EQ(cf.getCp(0), nullptr);
    EXPECT_EQ(cf.getCp(999), nullptr);
}

// ---------- ClassFile::cpAccesses ----------

TEST(ClassFileCpAccessesTest, IncludesThisSuperInterfacesAttributesFieldsMethods)
{
    CpUtf8  aName("Foo"), bName("Bar"), supName("Base"), ifaceName("Iface"), attrName("X");
    CpClass aCls(&aName), bCls(&bName), supCls(&supName), ifaceCls(&ifaceName);

    Field*  f = new Field({}, 0, &aName, &bName);   // 测试 vector<const Field*> 持有指针
    Method* m = new Method({}, 0, &aName, &bName);
    std::vector<const Field*>     fields{f};
    std::vector<const Method*>    methods{m};
    std::vector<const Attribute*> attrs{new DefaultAttribute(&attrName, {})};

    ClassFile cf(0,
                 52,
                 std::make_unique<ConstPool>(),
                 0,
                 &aCls,
                 &supCls,
                 {&ifaceCls},
                 fields,
                 methods,
                 attrs);
    auto      accesses = cf.cpAccesses();

    bool hasThis = false, hasSup = false, hasIface = false, hasAttr = false;
    for (const CpEntry* e : accesses) {
        if (e == &aCls) hasThis = true;
        if (e == &supCls) hasSup = true;
        if (e == &ifaceCls) hasIface = true;
        if (e == &attrName) hasAttr = true;
    }
    EXPECT_TRUE(hasThis);
    EXPECT_TRUE(hasSup);
    EXPECT_TRUE(hasIface);
    EXPECT_TRUE(hasAttr);

    delete f;
    delete m;
    for (const Attribute* a : attrs) delete a;
}

TEST(ClassFileCpAccessesTest, NullSuperIsOmitted)
{
    CpUtf8                        aName("Foo"), attrName("X");
    CpClass                       aCls(&aName);
    std::vector<const Attribute*> attrs{new DefaultAttribute(&attrName, {})};
    ClassFile cf(0, 52, std::make_unique<ConstPool>(), 0, &aCls, nullptr, {}, {}, {}, attrs);
    auto      accesses = cf.cpAccesses();
    EXPECT_FALSE(accesses.empty());
    EXPECT_EQ(std::count(accesses.begin(), accesses.end(), static_cast<const CpEntry*>(&aCls)), 1);
    for (const Attribute* a : attrs) delete a;
}

// ---------- ClassMember ----------

TEST(ClassMemberTest, FieldHolderType)
{
    CpUtf8 n("f"), t("I");
    Field  f({}, 0, &n, &t);
    EXPECT_EQ(f.holderType(), AttributeHolderType::FIELD);
    EXPECT_EQ(f.name()->text(), "f");
    EXPECT_EQ(f.type()->text(), "I");
}

TEST(ClassMemberTest, MethodHolderType)
{
    CpUtf8 n("m"), t("()V");
    Method m({}, 0, &n, &t);
    EXPECT_EQ(m.holderType(), AttributeHolderType::METHOD);
    EXPECT_EQ(m.name()->text(), "m");
    EXPECT_EQ(m.type()->text(), "()V");
}

TEST(ClassMemberTest, ToStringFormats)
{
    CpUtf8 n("count"), t("I");
    Field  f({}, 0, &n, &t);
    EXPECT_EQ(f.toString(), "count I");

    CpUtf8 mn("doIt"), mt("()V");
    Method m({}, 0, &mn, &mt);
    EXPECT_EQ(m.toString(), "doIt()V");
}

TEST(ClassMemberTest, CpAccessesIncludesNameTypeAndAttributes)
{
    CpUtf8                        n("m"), t("()V"), attrName("X"), attrName2("Y");
    std::vector<const Attribute*> attrs{
        new DefaultAttribute(&attrName, {}),
        new DefaultAttribute(&attrName2, {}),
    };
    Method m(attrs, 0, &n, &t);
    auto   accesses = m.cpAccesses();
    EXPECT_GE(accesses.size(), 4u);
    EXPECT_NE(std::find(accesses.begin(), accesses.end(), static_cast<const CpEntry*>(&n)),
              accesses.end());
    EXPECT_NE(std::find(accesses.begin(), accesses.end(), static_cast<const CpEntry*>(&t)),
              accesses.end());
    EXPECT_NE(std::find(accesses.begin(), accesses.end(), static_cast<const CpEntry*>(&attrName)),
              accesses.end());
    EXPECT_NE(std::find(accesses.begin(), accesses.end(), static_cast<const CpEntry*>(&attrName2)),
              accesses.end());
    for (const Attribute* a : attrs) delete a;
}

TEST(ClassMemberTest, MethodSkipsCodeAttributeCpAccessesOnNative)
{
    CpUtf8               n("m"), t("()V"), codeName("Code");
    auto                 code    = makeCodeAttr(&codeName);
    const CodeAttribute* codeRaw = code.get();
    // Method 持有非拥有指针；将所有权转移给栈变量。
    std::vector<const Attribute*> attrs{codeRaw};
    constexpr int                 ACC_NATIVE = 0x0100;
    Method                        m(attrs, ACC_NATIVE, &n, &t);
    auto                          accesses = m.cpAccesses();
    // 期望：name + type，code 自身的 cpAccesses 应被跳过。
    EXPECT_EQ(accesses.size(), 2u);
    EXPECT_NE(std::find(accesses.begin(), accesses.end(), static_cast<const CpEntry*>(&n)),
              accesses.end());
    EXPECT_NE(std::find(accesses.begin(), accesses.end(), static_cast<const CpEntry*>(&t)),
              accesses.end());
}

TEST(ClassMemberTest, MethodSkipsCodeAttributeCpAccessesOnAbstract)
{
    CpUtf8                        n("m"), t("()V"), codeName("Code");
    auto                          code    = makeCodeAttr(&codeName);
    const CodeAttribute*          codeRaw = code.get();
    std::vector<const Attribute*> attrs{codeRaw};
    constexpr int                 ACC_ABSTRACT = 0x0400;
    Method                        m(attrs, ACC_ABSTRACT, &n, &t);
    auto                          accesses = m.cpAccesses();
    EXPECT_EQ(accesses.size(), 2u);
}

TEST(ClassMemberTest, MethodCollectsCodeAttributeCpAccessesOnPlain)
{
    CpUtf8                        n("m"), t("()V"), codeName("Code");
    auto                          code    = makeCodeAttr(&codeName);
    const CodeAttribute*          codeRaw = code.get();
    std::vector<const Attribute*> attrs{codeRaw};
    Method                        m(attrs, 0x0001 /*ACC_PUBLIC*/, &n, &t);
    auto                          accesses = m.cpAccesses();
    // name + type + code.cpAccesses (空，但至少 2 项)
    EXPECT_GE(accesses.size(), 2u);
}

TEST(ClassMemberTest, CodeAttributeIsCodeMarkerTrue)
{
    CpUtf8        codeName("Code");
    CodeAttribute ca(&codeName, 1, 1, {}, {}, {});
    EXPECT_TRUE(ca.isCodeAttribute());
    EXPECT_EQ(ca.holderType(), AttributeHolderType::ATTRIBUTE);
}

TEST(ClassMemberTest, NonCodeAttributeIsCodeMarkerFalse)
{
    CpUtf8           n("X");
    DefaultAttribute da(&n, {});
    EXPECT_FALSE(da.isCodeAttribute());
}

// ---------- ClassFile fields/methods/attributes mutation ----------

TEST(ClassFileTest, FieldsMethodsAttributesAccessors)
{
    CpUtf8    n("X");
    CpClass   c(&n);
    ClassFile cf(0, 52, std::make_unique<ConstPool>(), 0, &c, nullptr, {}, {}, {}, {});
    EXPECT_TRUE(cf.getFields().empty());
    EXPECT_TRUE(cf.getMethods().empty());
    EXPECT_TRUE(cf.attributes().empty());

    CpUtf8 fn("f"), ft("I");
    Field* f = new Field({}, 0, &fn, &ft);
    cf.setFields({f});
    EXPECT_EQ(cf.getFields().size(), 1u);
    EXPECT_EQ(cf.getFields()[0], f);
    delete f;
}

TEST(ClassFileTest, SetterUpdatesUnderlyingStorage)
{
    CpUtf8    n("X");
    CpClass   c(&n);
    ClassFile cf(0, 52, std::make_unique<ConstPool>(), 0, &c, nullptr, {}, {}, {}, {});

    std::vector<const CpClass*> ifaces{&c, &c};
    cf.setInterfaceClasses(ifaces);
    EXPECT_EQ(cf.getInterfaceClasses().size(), 2u);

    cf.setThisClass(&c);
    EXPECT_EQ(cf.getThisClass(), &c);
    cf.setSuperClass(&c);
    EXPECT_EQ(cf.getSuperClass(), &c);
}

// ---------- InvalidClassException ----------

TEST(InvalidClassExceptionTest, FromMessage)
{
    InvalidClassException ex("bad magic");
    EXPECT_STREQ(ex.what(), "bad magic");
}

TEST(InvalidClassExceptionTest, FromCauseException)
{
    std::runtime_error    cause("underlying io failure");
    InvalidClassException ex(cause);
    EXPECT_STREQ(ex.what(), "underlying io failure");
}

TEST(InvalidClassExceptionTest, IsStdExceptionSubclass)
{
    InvalidClassException ex("msg");
    std::exception*       base = &ex;
    EXPECT_NE(base->what(), nullptr);
}
