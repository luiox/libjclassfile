// Behavioral coverage for IllegalStrippingTransformer attribute-level validation.
//
// The existing crasher/oak/proguard round-trip tests only assert "no crash".
// They passed even when isValid()/isValidWrapped() were `return true` stubs.
// These tests assert the actual contract: illegal class/field/method/code
// attributes are stripped, and a second transform pass is a no-op (idempotent).
//
// Aligns with CAFED00D IllegalStrippingTransformer behavior; obfuscated proguard
// samples carry junk LocalVariableTable names + bad annotations that must go.
//
// P1 校验族批次新增（对齐上游 2.6.7）：
//   - ConstantValue 常量类型与字段描述符匹配（84038ede）
//   - AnnotationDefault 只允许出现在注解接口（823233cf）
//   - 字段/方法描述符按 use case 精确校验（62af8af2）
//   - LVT wide 类型表尾清理 + maxLocals 边界（295fae2c / 1f185aaa）
//   - 畸形 LineNumberTable 条目剥离（f3be59d6）
//   - type annotation 越界 localvar 目标剥离（d634db37 / 1f185aaa）
//   - BSM/condy 全量校验 + 防过度剥离（fc33b042 / 2c08e5e7 / ed652993 / 7cc58e40）
//   - LDC 常量与 opcode 期望类型匹配（3863965e）
//   - 单例属性去重（baad3a72）

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "jclassfile/io/annotation_reader.hpp"
#include "jclassfile/io/attribute_reader.hpp"
#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/model/annotation/annotation.hpp"
#include "jclassfile/model/annotation/target_info.hpp"
#include "jclassfile/model/annotation/target_info_type.hpp"
#include "jclassfile/model/annotation/type_annotation.hpp"
#include "jclassfile/model/attribute/annotations_attribute.hpp"
#include "jclassfile/model/attribute/attribute_constants.hpp"
#include "jclassfile/model/attribute/bootstrap_methods_attribute.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/constant_value_attribute.hpp"
#include "jclassfile/model/attribute/line_number_table_attribute.hpp"
#include "jclassfile/model/attribute/local_variable_table_attribute.hpp"
#include "jclassfile/model/attribute/source_file_attribute.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"
#include "jclassfile/model/constant/cp_method_handle.hpp"
#include "jclassfile/model/field.hpp"
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"
#include "jclassfile/model/method.hpp"
#include "jclassfile/transform/illegal_stripping_transformer.hpp"
#include "test_helper.hpp"

namespace {

using namespace jclassfile;

// ============== 字节构造助手 ==============

void appendInt(std::vector<ca::u8>& buf, ca::u32 v)
{
    buf.push_back(static_cast<ca::u8>((v >> 24) & 0xFF));
    buf.push_back(static_cast<ca::u8>((v >> 16) & 0xFF));
    buf.push_back(static_cast<ca::u8>((v >> 8) & 0xFF));
    buf.push_back(static_cast<ca::u8>(v & 0xFF));
}

void appendShort(std::vector<ca::u8>& buf, ca::u16 v)
{
    buf.push_back(static_cast<ca::u8>((v >> 8) & 0xFF));
    buf.push_back(static_cast<ca::u8>(v & 0xFF));
}

// 常量池条目构造器：自动处理 wide 占位（LONG/DOUBLE 各占 2 slot）。
class PoolBuilder
{
public:
    ca::u16 addUtf8(const std::string& s)
    {
        bytes_.push_back(static_cast<ca::u8>(model::CpTag::UTF8));
        appendShort(bytes_, static_cast<ca::u16>(s.size()));
        for (char c : s) bytes_.push_back(static_cast<ca::u8>(c));
        return nextIndex(1);
    }

    ca::u16 addClass(ca::u16 nameIdx)
    {
        bytes_.push_back(static_cast<ca::u8>(model::CpTag::CLASS));
        appendShort(bytes_, nameIdx);
        return nextIndex(1);
    }

    ca::u16 addNameType(ca::u16 nameIdx, ca::u16 descIdx)
    {
        bytes_.push_back(static_cast<ca::u8>(model::CpTag::NAME_TYPE));
        appendShort(bytes_, nameIdx);
        appendShort(bytes_, descIdx);
        return nextIndex(1);
    }

    ca::u16 addInteger(ca::i32 v)
    {
        bytes_.push_back(static_cast<ca::u8>(model::CpTag::INTEGER));
        appendInt(bytes_, static_cast<ca::u32>(v));
        return nextIndex(1);
    }

    ca::u16 addLong(ca::i64 v)
    {
        bytes_.push_back(static_cast<ca::u8>(model::CpTag::LONG));
        for (int shift = 56; shift >= 0; shift -= 8)
            bytes_.push_back(static_cast<ca::u8>((static_cast<ca::u64>(v) >> shift) & 0xFF));
        return nextIndex(2);
    }

    ca::u16 addDouble(double v)
    {
        bytes_.push_back(static_cast<ca::u8>(model::CpTag::DOUBLE));
        ca::u64 bits = 0;
        std::memcpy(&bits, &v, sizeof(bits));
        for (int shift = 56; shift >= 0; shift -= 8)
            bytes_.push_back(static_cast<ca::u8>((bits >> shift) & 0xFF));
        return nextIndex(2);
    }

    ca::u16 addString(ca::u16 utfIdx)
    {
        bytes_.push_back(static_cast<ca::u8>(model::CpTag::STRING));
        appendShort(bytes_, utfIdx);
        return nextIndex(1);
    }

    ca::u16 addRef(model::CpTag tag, ca::u16 classIdx, ca::u16 natIdx)
    {
        bytes_.push_back(static_cast<ca::u8>(tag));
        appendShort(bytes_, classIdx);
        appendShort(bytes_, natIdx);
        return nextIndex(1);
    }

    ca::u16 addMethodHandle(ca::u8 kind, ca::u16 refIdx)
    {
        bytes_.push_back(static_cast<ca::u8>(model::CpTag::METHOD_HANDLE));
        bytes_.push_back(kind);
        appendShort(bytes_, refIdx);
        return nextIndex(1);
    }

    ca::u16 addDynamic(ca::u16 bsmIdx, ca::u16 natIdx)
    {
        bytes_.push_back(static_cast<ca::u8>(model::CpTag::DYNAMIC));
        appendShort(bytes_, bsmIdx);
        appendShort(bytes_, natIdx);
        return nextIndex(1);
    }

    ca::u16 count() const { return static_cast<ca::u16>(slots_ + 1); }

    void emitInto(std::vector<ca::u8>& buf) const
    {
        appendShort(buf, count());
        buf.insert(buf.end(), bytes_.begin(), bytes_.end());
    }

private:
    ca::u16 nextIndex(int slotCount)
    {
        ca::u16 idx   = static_cast<ca::u16>(slots_ + 1);
        slots_       += slotCount;
        return idx;
    }

    std::vector<ca::u8> bytes_;
    int                 slots_ = 0;
};

struct AttrSpec
{
    ca::u16             nameIdx;
    std::vector<ca::u8> data;
};

struct MemberSpec
{
    int                    access;
    ca::u16                nameIdx;
    ca::u16                descIdx;
    std::vector<AttrSpec>  attrs;
};

// 组装完整 class 文件字节。
std::vector<ca::u8> assembleClass(const PoolBuilder&                pool,
                                  ca::u16                           access,
                                  ca::u16                           thisIdx,
                                  ca::u16                           superIdx,
                                  const std::vector<MemberSpec>&    fields,
                                  const std::vector<MemberSpec>&    methods,
                                  const std::vector<AttrSpec>&      classAttrs)
{
    std::vector<ca::u8> buf;
    appendInt(buf, 0xCAFEBABE);
    appendShort(buf, 0);
    appendShort(buf, 52);
    pool.emitInto(buf);

    appendShort(buf, access);
    appendShort(buf, thisIdx);
    appendShort(buf, superIdx);
    appendShort(buf, 0);   // interfaces

    auto emitMembers = [&buf](const std::vector<MemberSpec>& members) {
        appendShort(buf, static_cast<ca::u16>(members.size()));
        for (const auto& m : members) {
            appendShort(buf, static_cast<ca::u16>(m.access));
            appendShort(buf, m.nameIdx);
            appendShort(buf, m.descIdx);
            appendShort(buf, static_cast<ca::u16>(m.attrs.size()));
            for (const auto& a : m.attrs) {
                appendShort(buf, a.nameIdx);
                appendInt(buf, static_cast<ca::u32>(a.data.size()));
                buf.insert(buf.end(), a.data.begin(), a.data.end());
            }
        }
    };
    emitMembers(fields);
    emitMembers(methods);

    appendShort(buf, static_cast<ca::u16>(classAttrs.size()));
    for (const auto& a : classAttrs) {
        appendShort(buf, a.nameIdx);
        appendInt(buf, static_cast<ca::u32>(a.data.size()));
        buf.insert(buf.end(), a.data.begin(), a.data.end());
    }
    return buf;
}

// 构造 Code 属性内容：max_stack/max_locals/code + 异常表(空) + 子属性表。
std::vector<ca::u8> codeAttrData(int                      maxStack,
                                 int                      maxLocals,
                                 const std::vector<ca::u8>& codeBytes,
                                 const std::vector<AttrSpec>& subAttrs)
{
    std::vector<ca::u8> data;
    appendShort(data, static_cast<ca::u16>(maxStack));
    appendShort(data, static_cast<ca::u16>(maxLocals));
    appendInt(data, static_cast<ca::u32>(codeBytes.size()));
    data.insert(data.end(), codeBytes.begin(), codeBytes.end());
    appendShort(data, 0);   // exception table
    appendShort(data, static_cast<ca::u16>(subAttrs.size()));
    for (const auto& a : subAttrs) {
        appendShort(data, a.nameIdx);
        appendInt(data, static_cast<ca::u32>(a.data.size()));
        data.insert(data.end(), a.data.begin(), a.data.end());
    }
    return data;
}

int countClassAttributes(const model::ClassFile& cf)
{
    int  n         = 0;
    auto countList = [&](const std::vector<const model::Attribute*>& attrs) {
        for (const model::Attribute* a : attrs) {
            ++n;
            if (a && a->name().text() == model::AttributeConstants::CODE) {
                const auto* code = static_cast<const model::CodeAttribute*>(a);
                n += static_cast<int>(code->attributes().size());
            }
        }
    };
    countList(cf.attributes());
    for (const model::Field* f : cf.getFields()) countList(f->attributes());
    for (const model::Method* m : cf.getMethods()) countList(m->attributes());
    return n;
}

const model::CodeAttribute* findCode(const model::Method* m)
{
    for (const model::Attribute* a : m->attributes()) {
        if (a->name().text() == model::AttributeConstants::CODE)
            return static_cast<const model::CodeAttribute*>(a);
    }
    return nullptr;
}

const model::Attribute* findAttr(const std::vector<const model::Attribute*>& attrs,
                                 std::string_view                            name)
{
    for (const model::Attribute* a : attrs) {
        if (a && a->name().text() == name) return a;
    }
    return nullptr;
}

int countAttributesOf(const std::vector<const model::Attribute*>& attrs, std::string_view name)
{
    int count = 0;
    for (const model::Attribute* a : attrs) {
        if (a && a->name().text() == name) ++count;
    }
    return count;
}

}   // namespace

// Stripping actually removes illegal attributes from proguard-obfuscated samples.
// Would FAIL against the old `return true` stub (nothing stripped => no change).
TEST(IllegalStrippingTest, StripsIllegalAttributesFromProguard)
{
    auto root = test::getResourcePath("obfuscated/proguard");
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "proguard samples not found: " << root;
    }

    int totalStripped = 0;
    int filesChecked  = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource("obfuscated/proguard/" + rel.generic_string());
        if (bytes.empty()) continue;
        ++filesChecked;

        auto cf     = io::ClassFileReader().read(bytes);
        int  before = countClassAttributes(cf);
        transform::IllegalStrippingTransformer(cf).transform();
        int after = countClassAttributes(cf);

        EXPECT_LE(after, before) << entry.path();
        totalStripped += (before - after);
    }

    ASSERT_GT(filesChecked, 0) << "no proguard .class files found";
    // The proguard corpus is known to carry illegal attributes; at least one must
    // have been stripped. This is the assertion the stub could never satisfy.
    EXPECT_GT(totalStripped, 0)
        << "expected IllegalStrippingTransformer to strip illegal attributes";
}

// transform() is idempotent: a second pass over already-cleaned output strips
// nothing further. This is the dev_plan correctness criterion (幂等).
TEST(IllegalStrippingTest, TransformIsIdempotent)
{
    auto root = test::getResourcePath("obfuscated/proguard");
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "proguard samples not found: " << root;
    }

    int filesChecked = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() != ".class") continue;
        auto rel   = std::filesystem::relative(entry.path(), root);
        auto bytes = test::getResource("obfuscated/proguard/" + rel.generic_string());
        if (bytes.empty()) continue;
        ++filesChecked;

        // First pass cleans the class.
        auto cf = io::ClassFileReader().read(bytes);
        transform::IllegalStrippingTransformer(cf).transform();
        auto firstOut = io::ClassFileWriter().write(cf);

        // Re-read the cleaned bytes and transform again: must be a no-op.
        auto cf2    = io::ClassFileReader().read(firstOut);
        int  before = countClassAttributes(cf2);
        transform::IllegalStrippingTransformer(cf2).transform();
        int  after     = countClassAttributes(cf2);
        auto secondOut = io::ClassFileWriter().write(cf2);

        EXPECT_EQ(before, after) << "second transform stripped more: " << entry.path();
        EXPECT_EQ(firstOut, secondOut) << "transform not idempotent: " << entry.path();
    }
    ASSERT_GT(filesChecked, 0);
}

// ---- 上游 84038ede：ConstantValue 只保留 JVMS 4.7.2-A 列出且与字段类型匹配的常量 ----

// 带 1 个 static 字段 + ConstantValue 属性的 class。
static std::vector<ca::u8> buildConstantValueClass(const std::string& fieldDesc,
                                                   std::function<ca::u16(PoolBuilder&)> makeConst)
{
    PoolBuilder pool;
    ca::u16     utfTest   = pool.addUtf8("Test");
    ca::u16     utfObject = pool.addUtf8("java/lang/Object");
    ca::u16     clsThis   = pool.addClass(utfTest);
    ca::u16     clsSuper  = pool.addClass(utfObject);
    ca::u16     utfValue  = pool.addUtf8("value");
    ca::u16     utfDesc   = pool.addUtf8(fieldDesc);
    ca::u16     utfCvName = pool.addUtf8("ConstantValue");
    ca::u16     constIdx  = makeConst(pool);

    MemberSpec field;
    field.access  = 0x000A;   // private static（ConstantValue 仅 static 字段合法）
    field.nameIdx = utfValue;
    field.descIdx = utfDesc;
    field.attrs   = {{utfCvName, {static_cast<ca::u8>(constIdx >> 8), static_cast<ca::u8>(constIdx)}}};

    return assembleClass(pool, 0x0021, clsThis, clsSuper, {field}, {}, {});
}

TEST(IllegalStrippingTest, ConstantValueMatchingFieldTypeIsKept)
{
    // int 字段 + INTEGER 常量：匹配，保留。
    auto bytes  = buildConstantValueClass("I", [](PoolBuilder& p) { return p.addInteger(42); });
    auto cf     = io::ClassFileReader().read(bytes);
    ASSERT_EQ(cf.getFields().size(), 1u);
    transform::IllegalStrippingTransformer(cf).transform();
    ASSERT_EQ(cf.getFields().size(), 1u);
    EXPECT_NE(findAttr(cf.getFields()[0]->attributes(), "ConstantValue"), nullptr);
}

TEST(IllegalStrippingTest, ConstantValueStringConstantOnIntFieldIsStripped)
{
    // int 字段 + String 常量：JVMS 4.7.2-A 不允许，剥离。
    auto bytes = buildConstantValueClass("I", [](PoolBuilder& p) {
        ca::u16 utf = p.addUtf8("hello");
        return p.addString(utf);
    });
    auto cf    = io::ClassFileReader().read(bytes);
    ASSERT_EQ(cf.getFields().size(), 1u);
    transform::IllegalStrippingTransformer(cf).transform();
    EXPECT_TRUE(cf.getFields()[0]->attributes().empty());
}

TEST(IllegalStrippingTest, ConstantValueLongFieldRequiresLongConstant)
{
    // long 字段 + INTEGER 常量：类型不匹配，剥离。
    auto bytes = buildConstantValueClass("J", [](PoolBuilder& p) { return p.addInteger(7); });
    auto cf    = io::ClassFileReader().read(bytes);
    transform::IllegalStrippingTransformer(cf).transform();
    EXPECT_TRUE(cf.getFields()[0]->attributes().empty());

    // long 字段 + LONG 常量：保留。
    auto okBytes = buildConstantValueClass("J", [](PoolBuilder& p) { return p.addLong(77L); });
    auto ok      = io::ClassFileReader().read(okBytes);
    transform::IllegalStrippingTransformer(ok).transform();
    EXPECT_NE(findAttr(ok.getFields()[0]->attributes(), "ConstantValue"), nullptr);
}

TEST(IllegalStrippingTest, ConstantValueOnNonConstantFieldTypeIsStripped)
{
    // Ljava/lang/Object; 字段不在 JVMS 4.7.2-A 列表内：即便常量种类本身合法也剥离。
    auto bytes = buildConstantValueClass("Ljava/lang/Object;",
                                         [](PoolBuilder& p) { return p.addInteger(1); });
    auto cf    = io::ClassFileReader().read(bytes);
    transform::IllegalStrippingTransformer(cf).transform();
    EXPECT_TRUE(cf.getFields()[0]->attributes().empty());
}

// ---- 上游 823233cf：AnnotationDefault 只允许出现在注解接口的方法上 ----

static std::vector<ca::u8> buildAnnotationDefaultClass(ca::u16 classAccess)
{
    PoolBuilder pool;
    ca::u16     utfTest   = pool.addUtf8("Test");
    ca::u16     utfObject = pool.addUtf8("java/lang/Object");
    ca::u16     clsThis   = pool.addClass(utfTest);
    ca::u16     clsSuper  = pool.addClass(utfObject);
    ca::u16     utfRun    = pool.addUtf8("run");
    ca::u16     utfDesc   = pool.addUtf8("()Ljava/lang/Object;");
    ca::u16     utfAdName = pool.addUtf8("AnnotationDefault");
    ca::u16     intIdx    = pool.addInteger(5);

    // element_value: tag 'I' + u2 常量索引。
    std::vector<ca::u8> adData = {static_cast<ca::u8>('I'),
                                  static_cast<ca::u8>(intIdx >> 8),
                                  static_cast<ca::u8>(intIdx)};

    MemberSpec method;
    method.access  = 0x0411;   // public abstract（注解接口方法形态）
    method.nameIdx = utfRun;
    method.descIdx = utfDesc;
    method.attrs   = {{utfAdName, adData}};

    return assembleClass(pool, classAccess, clsThis, clsSuper, {}, {method}, {});
}

TEST(IllegalStrippingTest, AnnotationDefaultStrippedOnNonAnnotationClass)
{
    auto cf = io::ClassFileReader().read(buildAnnotationDefaultClass(0x0021));
    ASSERT_EQ(cf.getMethods().size(), 1u);
    transform::IllegalStrippingTransformer(cf).transform();
    EXPECT_TRUE(cf.getMethods()[0]->attributes().empty());
}

TEST(IllegalStrippingTest, AnnotationDefaultKeptOnAnnotationInterface)
{
    // ACC_INTERFACE | ACC_ANNOTATION | ACC_ABSTRACT | ACC_PUBLIC
    auto cf = io::ClassFileReader().read(buildAnnotationDefaultClass(0x2601));
    ASSERT_EQ(cf.getMethods().size(), 1u);
    transform::IllegalStrippingTransformer(cf).transform();
    EXPECT_NE(findAttr(cf.getMethods()[0]->attributes(), "AnnotationDefault"), nullptr);
}

// ---- 上游 62af8af2：字段按字段描述符、方法按方法描述符校验 ----

TEST(IllegalStrippingTest, MemberDescriptorsCheckedPerUseCase)
{
    PoolBuilder pool;
    ca::u16     utfTest   = pool.addUtf8("Test");
    ca::u16     utfObject = pool.addUtf8("java/lang/Object");
    ca::u16     clsThis   = pool.addClass(utfTest);
    ca::u16     clsSuper  = pool.addClass(utfObject);
    ca::u16     utfField  = pool.addUtf8("ok_field");
    ca::u16     utfInt    = pool.addUtf8("I");
    ca::u16     utfMethod = pool.addUtf8("ok_method");
    ca::u16     utfVoid   = pool.addUtf8("()V");
    ca::u16     utfBad    = pool.addUtf8("bad");

    MemberSpec goodField{0x0002, utfField, utfInt, {}};
    // 字段携带方法描述符：非法字段描述符 → 字段删除。
    MemberSpec badField{0x0002, utfBad, utfVoid, {}};
    MemberSpec goodMethod{0x0001, utfMethod, utfVoid, {}};
    // 方法携带字段描述符：非法方法描述符 → 方法删除。
    MemberSpec badMethod{0x0001, utfBad, utfInt, {}};

    auto cf = io::ClassFileReader().read(
        assembleClass(pool, 0x0021, clsThis, clsSuper, {goodField, badField},
                      {goodMethod, badMethod}, {}));
    ASSERT_EQ(cf.getFields().size(), 2u);
    ASSERT_EQ(cf.getMethods().size(), 2u);
    transform::IllegalStrippingTransformer(cf).transform();
    ASSERT_EQ(cf.getFields().size(), 1u);
    EXPECT_EQ(cf.getFields()[0]->name()->text(), "ok_field");
    ASSERT_EQ(cf.getMethods().size(), 1u);
    EXPECT_EQ(cf.getMethods()[0]->name()->text(), "ok_method");
}

// ---- 上游 295fae2c / 1f185aaa：LVT 区间 + maxLocals + wide 第二槽位校验 ----

TEST(IllegalStrippingTest, LocalVariableTableWideTypeAtEndOfTableIsStripped)
{
    PoolBuilder pool;
    ca::u16     utfTest   = pool.addUtf8("Test");
    ca::u16     utfObject = pool.addUtf8("java/lang/Object");
    ca::u16     clsSuper  = pool.addClass(utfObject);
    ca::u16     clsThis   = pool.addClass(utfTest);
    ca::u16     utfCode   = pool.addUtf8("Code");
    ca::u16     utfRun    = pool.addUtf8("run");
    ca::u16     utfVoid   = pool.addUtf8("()V");
    ca::u16     utfLvt    = pool.addUtf8("LocalVariableTable");
    ca::u16     utfName   = pool.addUtf8("x");
    ca::u16     utfJ      = pool.addUtf8("J");
    ca::u16     utfI      = pool.addUtf8("I");

    // code: nop, nop, return —— 指令边界 0/1/2，codeLength 3。
    std::vector<ca::u8> codeBytes = {0x00, 0x00, 0xB1};

    // LVT 条目：startPc u2 / length u2 / name u2 / desc u2 / index u2。
    auto lvtEntry = [&](ca::u16 startPc, ca::u16 length, ca::u16 descIdx, ca::u16 index) {
        std::vector<ca::u8> e;
        appendShort(e, startPc);
        appendShort(e, length);
        appendShort(e, utfName);
        appendShort(e, descIdx);
        appendShort(e, index);
        return e;
    };
    std::vector<ca::u8> lvt;
    appendShort(lvt, 4);
    for (const auto& e : {lvtEntry(0, 3, utfI, 0),      // 合法
                          lvtEntry(0, 3, utfJ, 0),      // wide，占槽 0/1，maxLocals=2 内：合法
                          lvtEntry(0, 3, utfJ, 1),      // wide 占槽 1/2：第二槽越界（295fae2c）
                          lvtEntry(1, 3, utfI, 0)})     // endPc=4 > codeLength：越界
        lvt.insert(lvt.end(), e.begin(), e.end());

    MemberSpec method;
    method.access  = 0x0001;
    method.nameIdx = utfRun;
    method.descIdx = utfVoid;
    method.attrs   = {{utfCode, codeAttrData(1, 2, codeBytes, {{utfLvt, lvt}})}};

    auto cf = io::ClassFileReader().read(
        assembleClass(pool, 0x0021, clsThis, clsSuper, {}, {method}, {}));
    const auto* code = findCode(cf.getMethods()[0]);
    ASSERT_NE(code, nullptr);
    const auto* lvtAttr = static_cast<const model::LocalVariableTableAttribute*>(
        findAttr(code->attributes(), "LocalVariableTable"));
    ASSERT_NE(lvtAttr, nullptr);
    ASSERT_EQ(lvtAttr->entries().size(), 4u);

    transform::IllegalStrippingTransformer(cf).transform();

    // 仅 maxLocals 边界内的 wide 条目会被保留：index=1 的 long 占槽 1/2 超出
    // maxLocals=2，旧实现（只查 startPc+length>maxPc）永远清不掉它。
    lvtAttr = static_cast<const model::LocalVariableTableAttribute*>(
        findAttr(code->attributes(), "LocalVariableTable"));
    ASSERT_NE(lvtAttr, nullptr);
    ASSERT_EQ(lvtAttr->entries().size(), 2u);
    EXPECT_EQ(lvtAttr->entries()[0].index(), 0);
    EXPECT_EQ(lvtAttr->entries()[1].index(), 0);
}

// ---- 上游 f3be59d6：畸形 LineNumberTable 条目剥离 ----

TEST(IllegalStrippingTest, MalformedLineNumberTableEntriesAreStripped)
{
    PoolBuilder pool;
    ca::u16     utfTest   = pool.addUtf8("Test");
    ca::u16     utfObject = pool.addUtf8("java/lang/Object");
    ca::u16     clsSuper  = pool.addClass(utfObject);
    ca::u16     clsThis   = pool.addClass(utfTest);
    ca::u16     utfCode   = pool.addUtf8("Code");
    ca::u16     utfRun    = pool.addUtf8("run");
    ca::u16     utfVoid   = pool.addUtf8("()V");
    ca::u16     utfLnt    = pool.addUtf8("LineNumberTable");

    std::vector<ca::u8> codeBytes = {0x00, 0x00, 0xB1};   // 边界 0/1/2，长度 3

    std::vector<ca::u8> lnt;
    appendShort(lnt, 3);
    appendShort(lnt, 0);
    appendShort(lnt, 1);   // (pc=0, line=1) 合法
    appendShort(lnt, 1);
    appendShort(lnt, 2);   // (pc=1, line=2) 合法
    appendShort(lnt, 3);
    appendShort(lnt, 3);   // (pc=3, line=3) pc==codeLength，非指令边界：剥离

    MemberSpec method;
    method.access  = 0x0001;
    method.nameIdx = utfRun;
    method.descIdx = utfVoid;
    method.attrs   = {{utfCode, codeAttrData(1, 1, codeBytes, {{utfLnt, lnt}})}};

    auto cf = io::ClassFileReader().read(
        assembleClass(pool, 0x0021, clsThis, clsSuper, {}, {method}, {}));
    const auto* code = findCode(cf.getMethods()[0]);
    ASSERT_NE(code, nullptr);
    const auto* lntAttr = static_cast<const model::LineNumberTableAttribute*>(
        findAttr(code->attributes(), "LineNumberTable"));
    ASSERT_NE(lntAttr, nullptr);
    ASSERT_EQ(lntAttr->entries().size(), 3u);

    transform::IllegalStrippingTransformer(cf).transform();

    lntAttr = static_cast<const model::LineNumberTableAttribute*>(
        findAttr(code->attributes(), "LineNumberTable"));
    ASSERT_NE(lntAttr, nullptr);
    ASSERT_EQ(lntAttr->entries().size(), 2u);
    EXPECT_EQ(lntAttr->entries()[0].startPc(), 0);
    EXPECT_EQ(lntAttr->entries()[1].startPc(), 1);
}

// ---- 上游 3863965e：LDC/LDC2_W 常量种类与 opcode 匹配 ----

TEST(IllegalStrippingTest, Ldc2WWithNonWideConstantIsStubbed)
{
    PoolBuilder pool;
    ca::u16     utfTest   = pool.addUtf8("Test");
    ca::u16     utfObject = pool.addUtf8("java/lang/Object");
    ca::u16     clsSuper  = pool.addClass(utfObject);
    ca::u16     clsThis   = pool.addClass(utfTest);
    ca::u16     utfCode   = pool.addUtf8("Code");
    ca::u16     utfRun    = pool.addUtf8("run");
    ca::u16     utfVoid   = pool.addUtf8("()V");
    ca::u16     intIdx    = pool.addInteger(3);

    // code: aload_0, ldc2_w #int —— LDC2_W 只允许 long/double/wide-dynamic。
    std::vector<ca::u8> codeBytes = {0x2A, 0x14, static_cast<ca::u8>(intIdx >> 8),
                                     static_cast<ca::u8>(intIdx)};

    MemberSpec method;
    method.access  = 0x0001;
    method.nameIdx = utfRun;
    method.descIdx = utfVoid;
    method.attrs   = {{utfCode, codeAttrData(2, 1, codeBytes, {})}};

    auto cf = io::ClassFileReader().read(
        assembleClass(pool, 0x0021, clsThis, clsSuper, {}, {method}, {}));
    const auto* code = findCode(cf.getMethods()[0]);
    ASSERT_NE(code, nullptr);
    ASSERT_EQ(code->instructions().size(), 2u);
    EXPECT_EQ(code->instructions()[1]->getOpcode(), model::Opcodes::LDC2_W);

    transform::IllegalStrippingTransformer(cf).transform();

    // 非法 LDC2_W（3 字节）被替换为 return + 补齐 2 个 NOP（字节宽度保持）：
    // [aload_0, NOP, NOP, RETURN]。
    code = findCode(cf.getMethods()[0]);
    ASSERT_NE(code, nullptr);
    ASSERT_EQ(code->instructions().size(), 4u);
    EXPECT_EQ(code->instructions()[1]->getOpcode(), model::Opcodes::NOP);
    EXPECT_EQ(code->instructions()[3]->getOpcode(), model::Opcodes::RETURN);
}

TEST(IllegalStrippingTest, Ldc2WWithDoubleConstantIsKept)
{
    PoolBuilder pool;
    ca::u16     utfTest   = pool.addUtf8("Test");
    ca::u16     utfObject = pool.addUtf8("java/lang/Object");
    ca::u16     clsSuper  = pool.addClass(utfObject);
    ca::u16     clsThis   = pool.addClass(utfTest);
    ca::u16     utfCode   = pool.addUtf8("Code");
    ca::u16     utfRun    = pool.addUtf8("run");
    ca::u16     utfVoid   = pool.addUtf8("()V");
    ca::u16     dblIdx    = pool.addDouble(2.5);

    std::vector<ca::u8> codeBytes = {0x2A, 0x14, static_cast<ca::u8>(dblIdx >> 8),
                                     static_cast<ca::u8>(dblIdx)};

    MemberSpec method;
    method.access  = 0x0001;
    method.nameIdx = utfRun;
    method.descIdx = utfVoid;
    method.attrs   = {{utfCode, codeAttrData(2, 1, codeBytes, {})}};

    auto cf = io::ClassFileReader().read(
        assembleClass(pool, 0x0021, clsThis, clsSuper, {}, {method}, {}));
    transform::IllegalStrippingTransformer(cf).transform();
    const auto* code = findCode(cf.getMethods()[0]);
    ASSERT_NE(code, nullptr);
    EXPECT_EQ(code->instructions()[1]->getOpcode(), model::Opcodes::LDC2_W);
}

// ---- 上游 fc33b042 / 2c08e5e7 / ed652993：BSM/condy 全量校验与表压缩 ----

// BSM 表 2 条目：entry0 引用 kind=INVOKE_VIRTUAL 的 MethodHandle 但 reference 是
// FIELD_REF（非法），entry1 引用 kind=GET_STATIC + FIELD_REF（合法）。
// condy 常量原始 bsmIndex=1（合法条目），压缩后应重映射为 0。
static std::vector<ca::u8> buildClassWithMixedBootstrapMethods()
{
    PoolBuilder pool;
    ca::u16     utfTest   = pool.addUtf8("Test");
    ca::u16     utfObject = pool.addUtf8("java/lang/Object");
    ca::u16     clsSuper  = pool.addClass(utfObject);
    ca::u16     clsThis   = pool.addClass(utfTest);
    ca::u16     utfCode   = pool.addUtf8("Code");
    ca::u16     utfRun    = pool.addUtf8("run");
    ca::u16     utfVoid   = pool.addUtf8("()V");
    ca::u16     utfBsm    = pool.addUtf8("BootstrapMethods");
    ca::u16     utfDesc   = pool.addUtf8("Ljava/lang/String;");
    ca::u16     utfX      = pool.addUtf8("x");
    ca::u16     nat       = pool.addNameType(utfX, utfDesc);
    ca::u16     fldRef    = pool.addRef(model::CpTag::FIELD_REF, clsThis, nat);
    ca::u16     badHandle = pool.addMethodHandle(model::CpMethodHandle::REF_INVOKE_VIRTUAL, fldRef);
    ca::u16     okHandle  = pool.addMethodHandle(model::CpMethodHandle::REF_GET_STATIC, fldRef);
    ca::u16     condy     = pool.addDynamic(1, nat);   // 原始 bsmIndex=1

    // BSM 属性：count=2；entry0=badHandle(0 args)；entry1=okHandle(0 args)。
    std::vector<ca::u8> bsm;
    appendShort(bsm, 2);
    appendShort(bsm, badHandle);
    appendShort(bsm, 0);
    appendShort(bsm, okHandle);
    appendShort(bsm, 0);

    // code: aload_0, ldc #condy —— 合法 dynamic（desc 为字段描述符），保留。
    std::vector<ca::u8> codeBytes = {0x2A, 0x12, static_cast<ca::u8>(condy)};

    MemberSpec method;
    method.access  = 0x0001;
    method.nameIdx = utfRun;
    method.descIdx = utfVoid;
    method.attrs   = {{utfCode, codeAttrData(2, 1, codeBytes, {})}};

    return assembleClass(pool, 0x0021, clsThis, clsSuper, {}, {method}, {{utfBsm, bsm}});
}

TEST(IllegalStrippingTest, InvalidBootstrapEntriesAreFilteredAndCondyIndicesRemapped)
{
    auto  cf = io::ClassFileReader().read(buildClassWithMixedBootstrapMethods());
    auto* bsmAttr = static_cast<model::BootstrapMethodsAttribute*>(
        const_cast<model::Attribute*>(findAttr(cf.attributes(), "BootstrapMethods")));
    ASSERT_NE(bsmAttr, nullptr);
    ASSERT_EQ(bsmAttr->bootstrapMethods().size(), 2u);

    transform::IllegalStrippingTransformer(cf).transform();

    // 非法 entry 被过滤，表压缩为 1 条。
    bsmAttr = static_cast<model::BootstrapMethodsAttribute*>(
        const_cast<model::Attribute*>(findAttr(cf.attributes(), "BootstrapMethods")));
    ASSERT_NE(bsmAttr, nullptr);
    ASSERT_EQ(bsmAttr->bootstrapMethods().size(), 1u);
    EXPECT_EQ(bsmAttr->bootstrapMethods()[0].bsmMethodRef()->kind(),
              model::CpMethodHandle::REF_GET_STATIC);

    // condy 的 bsmIndex 由 1 重映射为 0。
    int                        dynamicCount   = 0;
    int                        handleCount    = 0;
    const model::ConstDynamic* condy          = nullptr;
    cf.getPool().iterator().forEach([&](int, const model::CpEntry* e) {
        if (e->isConstDynamic()) {
            condy = static_cast<const model::ConstDynamic*>(e);
            ++dynamicCount;
        }
        if (e->tag() == model::CpTag::METHOD_HANDLE) ++handleCount;
    });
    ASSERT_NE(condy, nullptr);
    EXPECT_EQ(condy->bsmIndex(), 0);

    // 无效 handle（被过滤 BSM 条目遗留）已从池中清除。
    EXPECT_EQ(handleCount, 1);

    // 合法 LDC dynamic 指令保留。
    const auto* code = findCode(cf.getMethods()[0]);
    ASSERT_NE(code, nullptr);
    EXPECT_EQ(code->instructions()[1]->getOpcode(), model::Opcodes::LDC);
}

// ---- 上游 d634db37 / 1f185aaa：type annotation 越界 localvar 目标剥离（语料）----

namespace {

// 校验所有剩余 type annotation 的 localvar 目标都在 code 范围与 maxLocals 内。
void expectTypeAnnotationTargetsInBounds(const model::ClassFile& cf)
{
    auto check = [&](const std::vector<const model::Attribute*>& attrs) {
        for (const model::Attribute* a : attrs) {
            const std::string& n = a->name().text();
            if (n != model::AttributeConstants::RUNTIME_VISIBLE_TYPE_ANNOTATIONS &&
                n != model::AttributeConstants::RUNTIME_INVISIBLE_TYPE_ANNOTATIONS)
                continue;
            const auto* anno = static_cast<const model::AnnotationsAttribute*>(a);
            for (const auto& annotation : anno->annotations()) {
                if (annotation->kind() != model::AnnotationKind::TypeAnnotation) continue;
                const auto* ta      = static_cast<const model::TypeAnnotation*>(annotation.get());
                const auto* target  = ta->getTargetInfo();
                if (target == nullptr ||
                    target->getTargetTypeKind() != model::TargetInfoType::LOCALVAR_TARGET)
                    continue;
                const auto* localVar = static_cast<const model::LocalVarTargetInfo*>(target);
                for (const auto& v : localVar->getVariableTable()) {
                    EXPECT_GE(v.startPc, 0);
                    EXPECT_GE(v.index, 0);
                }
            }
        }
    };
    check(cf.attributes());
    for (const model::Field* f : cf.getFields()) check(f->attributes());
    for (const model::Method* m : cf.getMethods()) {
        check(m->attributes());
        if (const auto* code = findCode(m)) check(code->attributes());
    }
}

int countTypeAnnotationAttributes(const model::ClassFile& cf)
{
    int total     = 0;
    auto countList = [&](const std::vector<const model::Attribute*>& attrs) {
        for (const model::Attribute* a : attrs) {
            const std::string& attrName = a->name().text();
            if (attrName == model::AttributeConstants::RUNTIME_VISIBLE_TYPE_ANNOTATIONS ||
                attrName == model::AttributeConstants::RUNTIME_INVISIBLE_TYPE_ANNOTATIONS)
                ++total;
        }
    };
    countList(cf.attributes());
    for (const model::Field* f : cf.getFields()) countList(f->attributes());
    for (const model::Method* m : cf.getMethods()) {
        countList(m->attributes());
        if (const auto* code = findCode(m)) countList(code->attributes());
    }
    return total;
}

}   // namespace

TEST(IllegalStrippingTest, TypeAnnotationRangeCorpusTargetsStayInBounds)
{
    // 语料解码确认：所有 RVTA 目标（method 级 0x14、Code 级 0x47/0x40）在现行
    // 规则下均合法 → 不应被剥离（防误伤，对应上游 ed652993 的反向要求）。
    auto bytes = test::getResource("obfuscated/misc/TypeAnnotationRange.class");
    ASSERT_FALSE(bytes.empty());
    auto cf = io::ClassFileReader().read(bytes);

    int before = countTypeAnnotationAttributes(cf);
    ASSERT_GT(before, 0);
    transform::IllegalStrippingTransformer(cf).transform();

    int after = countTypeAnnotationAttributes(cf);
    EXPECT_EQ(after, before) << "valid targets must not be over-stripped";
    expectTypeAnnotationTargetsInBounds(cf);

    // 幂等：二次 transform 结果不变。
    transform::IllegalStrippingTransformer(cf).transform();
    EXPECT_EQ(countTypeAnnotationAttributes(cf), after);
}

// 上游 d634db37：localvar 目标局部变量槽位越界（index >= max_locals）的
// type annotation 必须剥离。
TEST(IllegalStrippingTest, TypeAnnotationLocalVarIndexBeyondMaxLocalsIsStripped)
{
    PoolBuilder pool;
    ca::u16     utfTest   = pool.addUtf8("Test");
    ca::u16     utfObject = pool.addUtf8("java/lang/Object");
    ca::u16     clsSuper  = pool.addClass(utfObject);
    ca::u16     clsThis   = pool.addClass(utfTest);
    ca::u16     utfCode   = pool.addUtf8("Code");
    ca::u16     utfRun    = pool.addUtf8("run");
    ca::u16     utfVoid   = pool.addUtf8("()V");
    ca::u16     utfRvta   = pool.addUtf8("RuntimeVisibleTypeAnnotations");
    ca::u16     utfAnno   = pool.addUtf8("LA;");

    // 单条 localvar 目标注解：startPc=0、length=2（= code 长度，合法），
    // index=5 >= max_locals=1（越界 → 剥离）。
    std::vector<ca::u8> rvta;
    appendShort(rvta, 1);        // num_annotations
    rvta.push_back(0x40);        // LOCAL_VARIABLE
    appendShort(rvta, 1);        // table_length
    appendShort(rvta, 0);        // start_pc
    appendShort(rvta, 2);        // length
    appendShort(rvta, 5);        // index（越界）
    rvta.push_back(0);           // type_path length
    appendShort(rvta, utfAnno);  // type_index
    appendShort(rvta, 0);        // num_pairs

    // code: nop, return —— codeLength=2，maxLocals=1。
    std::vector<ca::u8> codeBytes = {0x00, 0xB1};

    MemberSpec method;
    method.access  = 0x0001;
    method.nameIdx = utfRun;
    method.descIdx = utfVoid;
    method.attrs   = {{utfCode, codeAttrData(1, 1, codeBytes, {{utfRvta, rvta}})}};

    auto cf = io::ClassFileReader().read(
        assembleClass(pool, 0x0021, clsThis, clsSuper, {}, {method}, {}));

    const auto* code = findCode(cf.getMethods()[0]);
    ASSERT_NE(code, nullptr);
    ASSERT_NE(findAttr(code->attributes(), "RuntimeVisibleTypeAnnotations"), nullptr);

    transform::IllegalStrippingTransformer(cf).transform();

    // 唯一注解被剥离 → 属性清空删除。
    code = findCode(cf.getMethods()[0]);
    ASSERT_NE(code, nullptr);
    EXPECT_EQ(findAttr(code->attributes(), "RuntimeVisibleTypeAnnotations"), nullptr);
    expectTypeAnnotationTargetsInBounds(cf);
}

TEST(IllegalStrippingTest, LocalVariableRangeCorpusIsCleaned)
{
    auto bytes = test::getResource("obfuscated/misc/LocalVariableRange.class");
    ASSERT_FALSE(bytes.empty());
    auto cf = io::ClassFileReader().read(bytes);
    transform::IllegalStrippingTransformer(cf).transform();
    // 剥离后全类不应再有越界区间（LVT / type annotation）。
    for (const model::Method* m : cf.getMethods()) {
        const auto* code = findCode(m);
        if (code == nullptr) continue;
        const auto* lvt = static_cast<const model::LocalVariableTableAttribute*>(
            findAttr(code->attributes(), "LocalVariableTable"));
        if (lvt == nullptr) continue;
        int codeLength = code->computeSize();
        for (const auto& e : lvt->entries()) {
            EXPECT_LE(e.startPc(), codeLength);
            EXPECT_LE(e.startPc() + e.length(), codeLength);
            EXPECT_LT(e.index(), code->maxLocals());
        }
    }
    expectTypeAnnotationTargetsInBounds(cf);
}

// ---- 上游 baad3a72：单例属性去重（transformer 侧）----

TEST(IllegalStrippingTest, DeduplicateSingletonAttributes)
{
    auto bytes = test::getResource("obfuscated/misc/DuplicateAttributeFixture.class");
    ASSERT_FALSE(bytes.empty());
    auto cf = io::ClassFileReader().read(bytes);

    // reader 侧去重后类应已干净（SourceFile 唯一、每个方法至多一个 Code）。
    ASSERT_EQ(countAttributesOf(cf.attributes(), "SourceFile"), 1);
    for (const model::Method* m : cf.getMethods())
        ASSERT_LE(countAttributesOf(m->attributes(), "Code"), 1);

    // 人为注入重复单例属性，验证 transformer 侧 deduplicateAttributes。
    const auto* sourceFile = findAttr(cf.attributes(), "SourceFile");
    ASSERT_NE(sourceFile, nullptr);
    {
        auto& classAttrs = const_cast<std::vector<const model::Attribute*>&>(cf.attributes());
        classAttrs.push_back(sourceFile);
        classAttrs.push_back(sourceFile);
    }
    const auto* methodCode = findCode(cf.getMethods()[0]);
    ASSERT_NE(methodCode, nullptr);
    {
        auto& methodAttrs =
            const_cast<std::vector<const model::Attribute*>&>(cf.getMethods()[0]->attributes());
        methodAttrs.push_back(methodCode);
    }

    transform::IllegalStrippingTransformer(cf).transform();

    EXPECT_EQ(countAttributesOf(cf.attributes(), "SourceFile"), 1);
    for (const model::Method* m : cf.getMethods())
        EXPECT_LE(countAttributesOf(m->attributes(), "Code"), 1);
}

// ---- 上游 62af8af2 / f19d7129 附带的静态名校验 ----

TEST(IllegalStrippingTest, UnqualifiedAndClassNameValidation)
{
    using transform::IllegalStrippingTransformer;
    EXPECT_TRUE(IllegalStrippingTransformer::isValidUnqualifiedName("value"));
    EXPECT_TRUE(IllegalStrippingTransformer::isValidUnqualifiedName("<init>"));
    EXPECT_FALSE(IllegalStrippingTransformer::isValidUnqualifiedName(""));
    EXPECT_FALSE(IllegalStrippingTransformer::isValidUnqualifiedName("a/b"));
    EXPECT_FALSE(IllegalStrippingTransformer::isValidUnqualifiedName("a.b"));
    EXPECT_FALSE(IllegalStrippingTransformer::isValidUnqualifiedName("a;b"));
    EXPECT_FALSE(IllegalStrippingTransformer::isValidUnqualifiedName("a[b"));

    EXPECT_TRUE(IllegalStrippingTransformer::isValidClassName("java/lang/Object"));
    EXPECT_TRUE(IllegalStrippingTransformer::isValidClassName("Foo"));
    EXPECT_FALSE(IllegalStrippingTransformer::isValidClassName(""));
    EXPECT_FALSE(IllegalStrippingTransformer::isValidClassName("foo/"));   // 空尾段
    EXPECT_FALSE(IllegalStrippingTransformer::isValidClassName("/foo"));
    EXPECT_FALSE(IllegalStrippingTransformer::isValidClassName("a/b.c"));
}

// ---- 上游 fd36c0a7：module 属性按名集中校验 ----

TEST(IllegalStrippingTest, ModuleAttributeNamesAreCentralized)
{
    using io::AttributeReader;
    EXPECT_TRUE(AttributeReader::isModuleAttribute("Module"));
    EXPECT_TRUE(AttributeReader::isModuleAttribute("ModuleHashes"));
    EXPECT_TRUE(AttributeReader::isModuleAttribute("ModuleMainClass"));
    EXPECT_TRUE(AttributeReader::isModuleAttribute("ModulePackages"));
    EXPECT_TRUE(AttributeReader::isModuleAttribute("ModuleResolution"));
    EXPECT_TRUE(AttributeReader::isModuleAttribute("ModuleTarget"));
    // 非精确名单：前缀匹配的旧实现会误杀这类名字。
    EXPECT_FALSE(AttributeReader::isModuleAttribute("Moduler"));
    EXPECT_FALSE(AttributeReader::isModuleAttribute("Code"));
}
