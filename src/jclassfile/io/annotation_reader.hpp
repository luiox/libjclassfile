// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/AnnotationReader.java
// 状态：阶段 6 完整实现
// 备注：
//   - 原版构造时切片出独立 DataInputStream（"is"），由父流 is 读 length 字节。
//     C++ 等价：拿父流引用、attributeLength，每次读 N 字节后通过 IndexableByteStream
//     的当前位置跟踪。
//   - 嵌套深度限制：MAX_NESTING = 50（与原版一致）。
//   - 异常处理：原版 try/catch Throwable → 静默丢弃（记日志），返回 null。
//     C++ 沿用：catch std::exception（标准异常），不引入日志（沿用原版契约）。
//   - AnnotationScope：原版是 ArrayList<String> 链式 with()。
//     C++ 用 std::vector<std::string> + push_back 实现。
//   - 公共读路径：readAnnotationDefault / readAnnotations / readParameterAnnotations
//     / readTypeAnnotations。
//   - 私有读路径：readAnnotation / readTypeAnnotation / readTypePath / readElementPairs
//     / readElementValue。
//   - 类型分派：读 elementValue 用 char tag 走 switch；读 target info 用 int 走 switch。
//   - 项目禁用 RTTI：用 ElementValue::kind() 枚举 + static_cast 替代 Java 的
//     instanceof + cast；用 TargetInfo::kind() 替代嵌套类 cast。
//   - typeIndex / nameIndex 必须指向 CpUtf8（Annotation / ElementPair 的语义约束）。
//     原版用 instanceof CpUtf8 校验；C++ 用 tag() == UTF8 + static_cast。

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/io/attribute_context.hpp"
#include "jclassfile/io/indexable_byte_stream.hpp"
#include "jclassfile/model/annotation/annotation.hpp"
#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/annotation/type_annotation.hpp"
#include "jclassfile/model/annotation/type_path.hpp"
#include "jclassfile/model/attribute/annotation_default_attribute.hpp"
#include "jclassfile/model/attribute/annotations_attribute.hpp"
#include "jclassfile/model/attribute/parameter_annotations_attribute.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::io {

class ClassFileReader;
class ClassBuilder;
class AnnotationReader
{
public:
    static constexpr int MAX_NESTING = 50;

    /// @brief 原始类型 element value 的 tag 反查必须的常量池条目 tag
    /// （JVMS 4.7.16.1-A，上游 66d31cf8）。
    ///
    /// @param tag 原始类型元素值的 ASCII tag（B/C/D/F/I/J/S/Z）。
    /// @return 值必须使用的常量池 tag；非原始类型 tag 返回 -1。
    ///
    /// 上游位于 PrimitiveElementValue.requiredConstantTag；model 文件不在本次
    /// 改动范围内，暂以 AnnotationReader 静态方法承载，transformer 侧复用。
    [[nodiscard]] static int requiredConstantTagForPrimitive(char tag);

    AnnotationReader(ClassFileReader& reader, class ClassBuilder& builder,
                     IndexableByteStream& is, int length, const model::CpUtf8* name,
                     AttributeContext context, bool visible);

    /// @brief 读取 AnnotationDefault 属性。
    /// @return
    /// 解析成功的属性；宽容模式下遇到畸形内容时返回 nullptr。
    [[nodiscard]] model::AnnotationDefaultAttribute* readAnnotationDefault();

    /// @brief 读取 RuntimeVisibleAnnotations 或 RuntimeInvisibleAnnotations 属性。
    ///
    /// @return 解析成功的属性；任一注解结构无法完整读取时返回 nullptr。
    ///
    ///
    /// 常量池值引用允许暂存为空指针，交由 IllegalStrippingTransformer 删除整个属性；
    /// 不在
    /// reader 阶段伪造常量池索引。
    [[nodiscard]] model::AnnotationsAttribute* readAnnotations();

    /// @brief 读取运行时参数注解属性。
    /// @return
    /// 解析成功的属性；宽容模式下遇到畸形内容时返回 nullptr。
    [[nodiscard]] model::ParameterAnnotationsAttribute* readParameterAnnotations();

    /// @brief 读取运行时类型注解属性。
    /// @return
    /// 解析成功的属性；宽容模式下遇到畸形内容时返回 nullptr。
    [[nodiscard]] model::AnnotationsAttribute* readTypeAnnotations();

private:
    class AnnotationScope
    {
    public:
        AnnotationScope& with(const std::string& value)
        {
            items_.push_back(value);
            return *this;
        }
        [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }

    private:
        std::vector<std::string> items_;
    };

    /// 递归深度守卫：构造时 +1，析构时 -1。depth_ 是 readElementValue 的
    /// 实际调用栈深度（AnnotationScope 因 with() 可变累积语义会统计兄弟
    /// 元素，不能用作栈深度度量）。
    class DepthGuard
    {
    public:
        explicit DepthGuard(AnnotationReader& reader)
            : reader_(reader)
        {
            ++reader_.depth_;
        }
        ~DepthGuard() { --reader_.depth_; }

        DepthGuard(const DepthGuard&)            = delete;
        DepthGuard& operator=(const DepthGuard&) = delete;

    private:
        AnnotationReader& reader_;
    };

    [[nodiscard]] std::unique_ptr<model::Annotation>     readAnnotation(AnnotationScope& scope);
    [[nodiscard]] std::unique_ptr<model::TypeAnnotation> readTypeAnnotation(AnnotationScope& scope);
    [[nodiscard]] model::TypePath                        readTypePath();
    [[nodiscard]] std::map<const model::CpUtf8*, std::unique_ptr<model::ElementValue>>
                                                       readElementPairs(AnnotationScope& scope);
    [[nodiscard]] std::unique_ptr<model::ElementValue> readElementValue(AnnotationScope& scope);

    ClassFileReader&             reader_;
    ClassBuilder&                builder_;
    const model::ConstPool&      cp_;
    IndexableByteStream&         parentIs_;
    int                          length_;
    const model::CpUtf8*         name_;
    AttributeContext             context_;
    int                          maxCpIndex_;
    bool                         visible_;
    int                          depth_ = 0;
};

}   // namespace jclassfile::io
