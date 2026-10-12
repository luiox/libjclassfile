// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedeu/io/AttributeWriter.java
// 状态：阶段 5c + 阶段 6 完整实现
// 备注：属性名是 Attribute 的稳定分派契约。实现根据该契约使用 static_cast，
// 不依赖 RTTI；构造模型时必须保持属性名和具体 Attribute 类型一致。

#pragma once

#include <memory>
#include <vector>

#include "jclassfile/io/fallback_instruction_writer.hpp"
#include "libca/core/datatype.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/attribute/stack_map_table_attribute.hpp"

namespace jclassfile::io {

/// @brief 将结构化 Attribute 序列化为 classfile 属性字节。
///
/// 属性的 name()
/// 决定其具体类型和二进制布局。写入器据此分派并使用 static_cast，
///
/// 因而调用方不得传入名称与实际类型不匹配的 Attribute。
class AttributeWriter
{
public:
    /// @brief 接管 fallback 指令写入器的所有权。
    explicit AttributeWriter(std::unique_ptr<FallbackInstructionWriter> fallback)
        : ownedFallback_(std::move(fallback))
        , fallback_(ownedFallback_.get())
    {}

    /// @brief 借用由调用方管理生命周期的 fallback 指令写入器。
    explicit AttributeWriter(FallbackInstructionWriter* fallback)
        : fallback_(fallback)
    {}

    /// @brief 使用会报告不支持指令的默认 fallback 写入器。
    AttributeWriter()
        : AttributeWriter(FallbackInstructionWriter::fail())
    {}

    /// @brief 写入完整 attribute_info（name_index、attribute_length 与属性体）。
    ///
    /// @param attribute 名称和具体类型一致的属性模型。
    /// @return 可直接写入 classfile
    /// 的属性字节。
    /// @throws std::invalid_argument 属性内容缺少必要常量池引用时抛出。

    [[nodiscard]] std::vector<ca::u8> writeAttribute(const model::Attribute& attribute);

private:
    [[nodiscard]] int orZero(const model::CpEntry* entry) const noexcept;

    std::unique_ptr<FallbackInstructionWriter> ownedFallback_;
    FallbackInstructionWriter*                 fallback_ = nullptr;
};

/// @brief 写入 StackMapTable 的 verification_type_info。
///
/// 供 StackMapTable
/// 属性序列化复用；TypeInfo::kind() 决定其具体派生类型。
void writeVerificationType(util::GrowingByteBuffer&                       out,
                           const model::StackMapTableAttribute::TypeInfo& type);

}   // namespace jclassfile::io
