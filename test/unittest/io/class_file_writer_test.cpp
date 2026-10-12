#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "jclassfile/invalid_class_exception.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

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
