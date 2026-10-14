#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "jclassfile/invalid_class_exception.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/instruction/cp_ref_instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"

using namespace jclassfile;
using namespace jclassfile::io;
using namespace jclassfile::model;

namespace {

ClassFile makeMinimalClass(std::string className)
{
    auto pool = std::make_unique<ConstPool>();

    auto    utf8Name    = std::make_unique<CpUtf8>(std::move(className));
    CpUtf8* utf8NamePtr = utf8Name.get();
    pool->add(std::move(utf8Name));

    auto     classEntry    = std::make_unique<CpClass>(utf8NamePtr);
    CpClass* classEntryPtr = classEntry.get();
    pool->add(std::move(classEntry));

    return ClassFile(0, 52, std::move(pool), 0x0021, classEntryPtr, nullptr, {}, {}, {}, {});
}

}   // namespace

TEST(ClassFileWriterTest, RejectsUtf8EntryLongerThanU16EncodedLength)
{
    ClassFile       cf = makeMinimalClass(std::string(70000, 'A'));
    ClassFileWriter writer;
    EXPECT_THROW((void)writer.write(cf), InvalidClassException);
}

// ---------- CpRef 指令写路径前置校验 ----------

// 构造只带 1 个 method（Code: 单条指令）的 ClassFile，指令由测试给定。
static ClassFile makeClassWithInsn(std::unique_ptr<model::Instruction> insn,
                                   CpUtf8&                            codeName,
                                   CpUtf8&                            mn,
                                   CpUtf8&                            mt)
{
    ClassFile cf = makeMinimalClass("T");
    auto*     insnView = cf.adoptInstruction(std::move(insn));
    auto code = std::make_unique<CodeAttribute>(&codeName,
                                                /*maxStack*/ 1,
                                                /*maxLocals*/ 1,
                                                std::vector<const Instruction*>{insnView},
                                                std::vector<CodeAttribute::ExceptionTableEntry>{},
                                                std::vector<const Attribute*>{});
    auto* codeView = cf.adoptAttribute(std::move(code));
    auto  method =
        std::make_unique<Method>(std::vector<const Attribute*>{codeView}, 0x0001, &mn, &mt);
    auto* methodView = cf.adoptMethod(std::move(method));
    cf.setMethods({methodView});
    return cf;
}

// LDC 的 entry 为 null（畸形输入穿透 / 上游改写残留）：写路径前置抛
// InvalidClassException，不再空指针解引用。
TEST(ClassFileWriterTest, ThrowsOnLdcWithNullCpEntry)
{
    CpUtf8 codeName("Code"), mn("m"), mt("()V");
    ClassFile cf = makeClassWithInsn(
        std::make_unique<CpRefInstruction>(Opcodes::LDC, static_cast<const CpEntry*>(nullptr)),
        codeName, mn, mt);
    ClassFileWriter writer;
    EXPECT_THROW((void)writer.write(cf), InvalidClassException);
}

// INVOKEINTERFACE 的 entry 非 ConstRef 族（此处给池里的 CpClass）：旧实现
// static_cast 后连解三级（nameType/type 纯垃圾解引用，UB）；修复后前置抛。
// ConstRef 族内（Field/Method/InterfaceMethodRef）不区分——InstructionRoundTrip
// 契约钉死 writer 只要求可解析出 nameType。
TEST(ClassFileWriterTest, ThrowsOnInvokeInterfaceWithNonConstRefEntry)
{
    CpUtf8 codeName("Code"), mn("m"), mt("()V");
    ClassFile    cf = makeMinimalClass("T");
    // makeMinimalClass 的池：#1 Utf8 "T"，#2 Class。取 Class 条目作错 tag 目标。
    const CpEntry* classEntry = cf.getCp(2);
    ASSERT_NE(classEntry, nullptr);
    ASSERT_EQ(classEntry->tag(), CpTag::CLASS);
    ClassFile bad = makeClassWithInsn(
        std::make_unique<CpRefInstruction>(Opcodes::INVOKEINTERFACE, classEntry), codeName, mn, mt);
    ClassFileWriter writer;
    EXPECT_THROW((void)writer.write(bad), InvalidClassException);
}
