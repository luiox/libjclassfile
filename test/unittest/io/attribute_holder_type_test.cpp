// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/AttributeHolderType.java
// 状态：阶段 1 单元测试
// 备注：原版 AttributeHolderTypeTest 不存在；本测试覆盖 5 个 enum 值的数值稳定性
//       （CAFED00D 内部直接用 ordinal 做协议分发，C++ 阶段 1 用 enum class 表达）。
//       上游 6489374e 起：attributeHolderTypeFromAnnotationTargetType 对未知 target
//       返回 nullopt；新增 isAnnotationTargetTypeValidIn 校验。

#include <gtest/gtest.h>

#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/model/annotation/annotation_constants.hpp"

using jclassfile::io::AttributeHolderType;
using jclassfile::model::AnnotationConstants;

TEST(AttributeHolderTypeTest, EnumValuesAreStable)
{
    EXPECT_EQ(static_cast<ca::u8>(AttributeHolderType::CLASS), 0);
    EXPECT_EQ(static_cast<ca::u8>(AttributeHolderType::FIELD), 1);
    EXPECT_EQ(static_cast<ca::u8>(AttributeHolderType::METHOD), 2);
    EXPECT_EQ(static_cast<ca::u8>(AttributeHolderType::RECORD_COMPONENT), 3);
    EXPECT_EQ(static_cast<ca::u8>(AttributeHolderType::ATTRIBUTE), 4);
}

TEST(AttributeHolderTypeTest, DistinctValuesAreNotEqual)
{
    EXPECT_NE(AttributeHolderType::CLASS, AttributeHolderType::FIELD);
    EXPECT_NE(AttributeHolderType::FIELD, AttributeHolderType::METHOD);
    EXPECT_NE(AttributeHolderType::RECORD_COMPONENT, AttributeHolderType::ATTRIBUTE);
}

// target_type → holder 映射（未知 target 返回 nullopt，不再抛异常）。
TEST(AttributeHolderTypeTest, FromAnnotationTargetTypeMapsKnownTargets)
{
    constexpr auto F = jclassfile::io::attributeHolderTypeFromAnnotationTargetType;

    EXPECT_EQ(F(AnnotationConstants::PARAMETER_OF_CLASS_OR_INTERFACE),
              std::optional(AttributeHolderType::CLASS));
    EXPECT_EQ(F(AnnotationConstants::SUPERTYPE), std::optional(AttributeHolderType::CLASS));
    EXPECT_EQ(F(AnnotationConstants::FIELD), std::optional(AttributeHolderType::FIELD));
    EXPECT_EQ(F(AnnotationConstants::METHOD_RETURN_TYPE), std::optional(AttributeHolderType::METHOD));
    EXPECT_EQ(F(AnnotationConstants::METHOD_THROWS), std::optional(AttributeHolderType::METHOD));
    EXPECT_EQ(F(AnnotationConstants::LOCAL_VARIABLE_DECLARATION),
              std::optional(AttributeHolderType::ATTRIBUTE));
    EXPECT_EQ(F(AnnotationConstants::CAST_EXPRESSION),
              std::optional(AttributeHolderType::ATTRIBUTE));
}

TEST(AttributeHolderTypeTest, FromAnnotationTargetTypeUnknownReturnsNullopt)
{
    constexpr auto F = jclassfile::io::attributeHolderTypeFromAnnotationTargetType;

    EXPECT_FALSE(F(0xFF).has_value());
    EXPECT_FALSE(F(0x2A).has_value());   // 未定义的 target 区段同样不抛异常
    EXPECT_TRUE(F(AnnotationConstants::FIELD).has_value());
}

// 上游 6489374e：0x13（FIELD 目标）同时用于 field 与 record component 声明。
TEST(AttributeHolderTypeTest, FieldTargetIsValidInRecordComponent)
{
    constexpr auto V = jclassfile::io::isAnnotationTargetTypeValidIn;

    EXPECT_TRUE(V(AnnotationConstants::FIELD, AttributeHolderType::RECORD_COMPONENT));
    EXPECT_TRUE(V(AnnotationConstants::FIELD, AttributeHolderType::FIELD));
    EXPECT_FALSE(V(AnnotationConstants::FIELD, AttributeHolderType::CLASS));
    EXPECT_FALSE(V(AnnotationConstants::FIELD, AttributeHolderType::METHOD));
}

TEST(AttributeHolderTypeTest, OtherTargetsAreStrictToSingleHolder)
{
    constexpr auto V = jclassfile::io::isAnnotationTargetTypeValidIn;

    EXPECT_TRUE(V(AnnotationConstants::SUPERTYPE, AttributeHolderType::CLASS));
    EXPECT_FALSE(V(AnnotationConstants::SUPERTYPE, AttributeHolderType::METHOD));

    EXPECT_TRUE(V(AnnotationConstants::METHOD_THROWS, AttributeHolderType::METHOD));
    EXPECT_FALSE(V(AnnotationConstants::METHOD_THROWS, AttributeHolderType::CLASS));

    EXPECT_TRUE(V(AnnotationConstants::INSTANCEOF_EXPRESSION, AttributeHolderType::ATTRIBUTE));
    EXPECT_FALSE(V(AnnotationConstants::INSTANCEOF_EXPRESSION, AttributeHolderType::FIELD));
}

// 未知 target：一律不匹配，且不得抛异常（安全丢弃由调用方处理）。
TEST(AttributeHolderTypeTest, UnknownTargetTypeIsNeverValid)
{
    constexpr auto V = jclassfile::io::isAnnotationTargetTypeValidIn;

    EXPECT_FALSE(V(0xFF, AttributeHolderType::CLASS));
    EXPECT_FALSE(V(0xFF, AttributeHolderType::FIELD));
    EXPECT_FALSE(V(0xFF, AttributeHolderType::METHOD));
    EXPECT_FALSE(V(0xFF, AttributeHolderType::RECORD_COMPONENT));
    EXPECT_FALSE(V(0xFF, AttributeHolderType::ATTRIBUTE));
    EXPECT_FALSE(V(0x7F, AttributeHolderType::RECORD_COMPONENT));
}
