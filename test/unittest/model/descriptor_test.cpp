// Descriptor 游标式解析契约（上游 62788a14：更多畸形判 Illegal）。
//
// 关键收紧点（旧实现宽松接受，现判 Illegal）：
//   - "V" 仅单独出现合法，"Vjunk"/"[V"/"(V)I" 非法；
//   - 整串必须完整消费，"Ijunk"/"(I)Vjunk"/"Ljava/lang/Object;garbage" 非法；
//   - 方法描述符顺序解析参数表而非只找第一个 ')'；
//   - 类名段不得含 '.'/'['/'<'/'>'（"Lfoo.bar;" 等非法）；
//   - JVMS 上限：数组 255 维、方法参数 255 槽位。

#include <gtest/gtest.h>

#include <string>

#include "jclassfile/model/descriptor.hpp"

using jclassfile::model::Descriptor;

namespace {

/// 重复 s 拼 n 次的辅助（对齐上游 String.repeat）。
std::string repeat(const std::string& s, int n)
{
    std::string out;
    out.reserve(s.size() * static_cast<std::size_t>(n));
    for (int i = 0; i < n; i++) out += s;
    return out;
}

}   // namespace

// ── 合法描述符 ──

TEST(DescriptorTest, parsesPrimitivesAndObjectAndArray)
{
    EXPECT_EQ(Descriptor::from("V").getKind(), Descriptor::Kind::Primitive);
    EXPECT_EQ(Descriptor::from("I").getKind(), Descriptor::Kind::Primitive);
    EXPECT_EQ(Descriptor::from("J").getKind(), Descriptor::Kind::Primitive);
    EXPECT_EQ(Descriptor::from("Ljava/lang/Object;").getKind(), Descriptor::Kind::Object);
    EXPECT_EQ(Descriptor::from("[I").getKind(), Descriptor::Kind::Array);
    EXPECT_EQ(Descriptor::from("[[I").getArrayLevel(), 2);
    EXPECT_EQ(Descriptor::from("[Ljava/lang/Object;").getKind(), Descriptor::Kind::Array);
}

TEST(DescriptorTest, parsesValidMethodDescriptors)
{
    const Descriptor d = Descriptor::from("(IJ)V");
    EXPECT_EQ(d.getKind(), Descriptor::Kind::Method);
    EXPECT_EQ(d.getParameterCount(), 2);
    EXPECT_EQ(Descriptor::from("()V").getKind(), Descriptor::Kind::Method);
    // wide 参数各占 2 槽位：J+D = 4。
    EXPECT_EQ(Descriptor::from("(JD)V").getParameterSize(), 4);
}

// ── 收紧后判非法的畸形（对齐上游 DescTest.testParseIllegal 新增行）──

TEST(DescriptorTest, marksTightenedCasesIllegal)
{
    const char* illegal[] = {
        "Vjunk",                        // 'V' 后尾随数据
        "Ijunk",                        // primitive 后尾随数据
        "Ljava/lang/Object;garbage",    // object 后尾随数据
        "[V",                           // void 不是字段类型
        "(L)V",                         // L 空类名
        "(L;)V",                        // 参数段空类名
        "(K)V",                         // 未知类型字符
        "Vjunk",
        "(I)Vjunk",                     // 方法尾随数据
        "(V)I",                         // void 不能作参数
        "([V)I",                        // void 不能作数组元素
        "Lfoo.bar;",                    // 类名含 '.'
        "Lfoo[bar;",                    // 类名含 '['
        "Lfoo<bar>;",                   // 类名含 '<' '>'
        "Lfoo",                         // 缺 ';'
        "(IJ",                          // 缺 ')'
        "()i",                          // 返回类型非法
        "(",                            // 空
        "",                             // 空串
    };
    for (const char* desc : illegal) {
        EXPECT_EQ(Descriptor::from(desc).getKind(), Descriptor::Kind::Illegal) << desc;
    }
}

// ── JVMS 上限（对齐上游 DescTest.testLimits）──

TEST(DescriptorTest, enforcesArrayDepthAndParamUnitLimits)
{
    // 255 维是数组上限；256 维非法。
    EXPECT_EQ(Descriptor::from(repeat("[", 255) + "I").getKind(), Descriptor::Kind::Array);
    EXPECT_EQ(Descriptor::from(repeat("[", 256) + "I").getKind(), Descriptor::Kind::Illegal);

    // 127 个 long（254 槽位）+ 1 个 int 恰好 255 槽位；128 个 long（256）超限。
    EXPECT_EQ(Descriptor::from("(" + repeat("J", 127) + "I)V").getKind(),
              Descriptor::Kind::Method);
    EXPECT_EQ(Descriptor::from("(" + repeat("J", 128) + ")V").getKind(),
              Descriptor::Kind::Illegal);
}
