// 阶段 7：Descriptor 解析测试
// 对齐 CAFED00D DescriptorTest

#include <gtest/gtest.h>

#include "jclassfile/model/descriptor.hpp"

namespace {

using jclassfile::model::Descriptor;

TEST(DescriptorTest, BasicFieldDescriptors)
{
    EXPECT_EQ(Descriptor("B").getParameterSize(), 1);   // byte
    EXPECT_EQ(Descriptor("C").getParameterSize(), 1);   // char
    EXPECT_EQ(Descriptor("D").getParameterSize(), 2);   // double
    EXPECT_EQ(Descriptor("F").getParameterSize(), 1);   // float
    EXPECT_EQ(Descriptor("I").getParameterSize(), 1);   // int
    EXPECT_EQ(Descriptor("J").getParameterSize(), 2);   // long
    EXPECT_EQ(Descriptor("S").getParameterSize(), 1);   // short
    EXPECT_EQ(Descriptor("Z").getParameterSize(), 1);   // boolean
}

TEST(DescriptorTest, ObjectDescriptor)
{
    EXPECT_EQ(Descriptor("Ljava/lang/String;").getParameterSize(), 1);
    EXPECT_EQ(Descriptor("Ljava/lang/Object;").getParameterSize(), 1);
}

TEST(DescriptorTest, ArrayDescriptor)
{
    EXPECT_EQ(Descriptor("[I").getParameterSize(), 1);                     // int[]
    EXPECT_EQ(Descriptor("[[Ljava/lang/Object;").getParameterSize(), 1);   // Object[][]
    EXPECT_EQ(Descriptor("[B").getParameterSize(), 1);                     // byte[]
    // Array descriptors always take 1 slot (reference), regardless of dimensions
    EXPECT_EQ(Descriptor("[[[[I").getParameterSize(), 1);
}

TEST(DescriptorTest, MethodDescriptors)
{
    EXPECT_EQ(Descriptor("()V").getParameterSize(), 0);
    EXPECT_EQ(Descriptor("(I)V").getParameterSize(), 1);
    EXPECT_EQ(Descriptor("(II)V").getParameterSize(), 2);
    EXPECT_EQ(Descriptor("(Ljava/lang/String;I)V").getParameterSize(), 2);
    EXPECT_EQ(Descriptor("(JD)V").getParameterSize(), 4);   // long + double = 2+2
    EXPECT_EQ(Descriptor("(Ljava/lang/Object;[I)Ljava/lang/String;").getParameterSize(), 2);
}

TEST(DescriptorTest, ComplexMethodDescriptors)
{
    // Multiple params including arrays and objects
    EXPECT_EQ(Descriptor("([Ljava/lang/String;I[I)V").getParameterSize(), 3);
    EXPECT_EQ(Descriptor("(JJ)V").getParameterSize(), 4);   // two longs
    EXPECT_EQ(Descriptor("(DD)V").getParameterSize(), 4);   // two doubles
}

}   // namespace
