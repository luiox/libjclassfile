// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/AttributeContext.java
// 状态：阶段 1 单元测试
// 备注：原版无 AttributeContextTest；本测试覆盖 5 个 is* 谓词（含 isAttribute() 的
//       "type == ATTRIBUTE || isRecordComponent()" 行为，原版 AttributeContext.java:40）。

#include <gtest/gtest.h>

#include "jclassfile/io/attribute_context.hpp"

using jclassfile::io::AttributeContext;
using jclassfile::io::AttributeHolderType;

TEST(AttributeContextTest, IsClassTrueOnlyForClass)
{
    AttributeContext ctx(AttributeHolderType::CLASS, 0);
    EXPECT_TRUE(ctx.isClass());
    EXPECT_FALSE(ctx.isField());
    EXPECT_FALSE(ctx.isMethod());
    EXPECT_FALSE(ctx.isRecordComponent());
    EXPECT_FALSE(ctx.isAttribute());
}

TEST(AttributeContextTest, IsFieldTrueOnlyForField)
{
    AttributeContext ctx(AttributeHolderType::FIELD, 0x0009);
    EXPECT_TRUE(ctx.isField());
    EXPECT_FALSE(ctx.isClass());
    EXPECT_FALSE(ctx.isMethod());
    EXPECT_EQ(ctx.memberAccess(), 0x0009);
}

TEST(AttributeContextTest, IsMethodTrueOnlyForMethod)
{
    AttributeContext ctx(AttributeHolderType::METHOD, 0x0008);
    EXPECT_TRUE(ctx.isMethod());
    EXPECT_EQ(ctx.memberAccess(), 0x0008);
}

TEST(AttributeContextTest, IsRecordComponentTrueOnlyForRecordComponent)
{
    AttributeContext ctx(AttributeHolderType::RECORD_COMPONENT, 0);
    EXPECT_TRUE(ctx.isRecordComponent());
    EXPECT_FALSE(ctx.isClass());
    EXPECT_FALSE(ctx.isField());
    EXPECT_FALSE(ctx.isMethod());
}

TEST(AttributeContextTest, IsAttributeTrueForAttributeHolder)
{
    AttributeContext ctx(AttributeHolderType::ATTRIBUTE, 0);
    EXPECT_TRUE(ctx.isAttribute());
    EXPECT_FALSE(ctx.isClass());
    EXPECT_FALSE(ctx.isRecordComponent());
}

TEST(AttributeContextTest, IsAttributeTrueForRecordComponent)
{
    // 对应原版 AttributeContext.java:40: type == ATTRIBUTE || isRecordComponent()
    AttributeContext ctx(AttributeHolderType::RECORD_COMPONENT, 0);
    EXPECT_TRUE(ctx.isAttribute());
}

TEST(AttributeContextTest, NameReturnsTypeName)
{
    EXPECT_STREQ(AttributeContext(AttributeHolderType::CLASS, 0).name(), "CLASS");
    EXPECT_STREQ(AttributeContext(AttributeHolderType::FIELD, 0).name(), "FIELD");
    EXPECT_STREQ(AttributeContext(AttributeHolderType::METHOD, 0).name(), "METHOD");
    EXPECT_STREQ(AttributeContext(AttributeHolderType::RECORD_COMPONENT, 0).name(),
                 "RECORD_COMPONENT");
    EXPECT_STREQ(AttributeContext(AttributeHolderType::ATTRIBUTE, 0).name(), "ATTRIBUTE");
}

TEST(AttributeContextTest, MemberAccessStored)
{
    AttributeContext ctx(AttributeHolderType::FIELD, 0x4001);
    EXPECT_EQ(ctx.memberAccess(), 0x4001);
}
