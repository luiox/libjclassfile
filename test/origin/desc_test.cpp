// CAFED00D 1:1 mirror: DescTest.java
// Descriptor parsing tests: parameter count, field parsing, method kind, illegal strings.

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "jclassfile/model/descriptor.hpp"

namespace {

using jclassfile::model::Descriptor;

// CAFED00D testParseMethodParams
TEST(DescTest, ParseMethodParams)
{
    auto assertParamCount = [](int expected, const std::string& desc) {
        Descriptor d(desc);
        EXPECT_EQ(expected, d.getParameterCount()) << "desc=" << desc;
        EXPECT_EQ(expected, static_cast<int>(d.getParameters().size())) << "desc=" << desc;
    };

    assertParamCount(0, "()V");
    assertParamCount(1, "(I)V");
    assertParamCount(1, "(Ljava/lang/String;)V");
    assertParamCount(1, "([Ljava/lang/String;)V");
    assertParamCount(1, "([I)V");
    assertParamCount(1, "([[I)V");
    assertParamCount(2, "(II)V");
    assertParamCount(2, "(I[I)V");
    assertParamCount(2, "([I[I)V");
    assertParamCount(2, "(Ljava/lang/String;Ljava/lang/String;)V");
    assertParamCount(2, "(Ljava/lang/String;[Ljava/lang/String;)V");
    assertParamCount(2, "([Ljava/lang/String;[Ljava/lang/String;)V");
    assertParamCount(2, "(Ljava/lang/String;I)V");
    assertParamCount(2, "(I[Ljava/lang/String;)V");
    assertParamCount(2, "([BLjava/lang/String;)Ljava/util/List;");
    assertParamCount(3, "([BLjava/lang/String;[B)Ljava/util/List;");
    assertParamCount(3, "([Ljava/lang/String;I[Ljava/lang/String;)V");
    assertParamCount(4, "([BLjava/lang/String;[B[B)Ljava/util/List;");
    assertParamCount(4, "([BLjava/lang/String;[BB)Ljava/util/List;");
}

// CAFED00D testParseFields
TEST(DescTest, ParseFields)
{
    for (const auto& desc : {"I", "Ljava/lang/String;", "[I", "[Ljava/lang/String;"}) {
        Descriptor d(desc);
        EXPECT_NE(Descriptor::Kind::Illegal, d.getKind()) << "desc=" << desc;
    }
}

// CAFED00D testParseMethods
TEST(DescTest, ParseMethods)
{
    // Note: "([L\0;)V" must be constructed with explicit length to include the embedded null byte
    std::vector<std::string> methodDescs = {"([LLL;)V",
                                            "([L ;)V",
                                            std::string("([L\0;)V", 7),
                                            "(I)V",
                                            "(Ljava/lang/String;)V",
                                            "(Ljava/lang/String;)Ljava/lang/String;",
                                            "(Ljava/lang/String;I)V",
                                            "(ILjava/lang/String;)V",
                                            "([I[I[I)[[[I"};
    for (const auto& desc : methodDescs) {
        Descriptor d(desc);
        EXPECT_EQ(Descriptor::Kind::Method, d.getKind()) << "desc=" << desc;
    }
}

// CAFED00D testParseIllegal
TEST(DescTest, ParseIllegal)
{
    for (const auto& desc : {"",
                             " ",
                             "\t",
                             "\n",
                             "K",
                             "[",
                             "[[K",
                             "[[L;",
                             "L;",
                             "()",
                             "(",
                             "()[",
                             "([)V",
                             "([K)V",
                             "([L)V",
                             "([L;)V",
                             "(L)V",
                             "(L;)V",
                             "(K)V"}) {
        Descriptor d(desc);
        EXPECT_EQ(Descriptor::Kind::Illegal, d.getKind()) << "desc=\"" << desc << "\"";
    }
}

}   // namespace
