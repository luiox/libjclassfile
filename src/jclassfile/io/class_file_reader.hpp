// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/ClassFileReader.java
// 状态：阶段 4b 实现（结构 + 5 config flags + 2-pass pool + 字段/方法/属性读取）
// 备注：
//   - 阶段 4b：完整实现 read(byte[]) 主流程 + 2-pass pool reading + readField/readMethod。
//   - 阶段 4c：AttributeReader 是最小 stub（仅读 name + length + raw bytes，返回
//   DefaultAttribute）。
//   - 阶段 5：替换为完整 AttributeReader（含 38 个 attribute 分发 + 版本/上下文/EOF/dup 决策）。
//   - 阶段 5：替换 getFallbackInstructionReader() 为真实 FallbackInstructionReader。
//   - 阶段 8：构造 IllegalRewritingInstructionsReader + IllegalStrippingTransformer。
//   - 异常模型：原版 throw IOException / InvalidClassException；C++ 只用 InvalidClassException
//     作为对外契约（reader 内部用 std::runtime_error / std::logic_error 表达编程错误）。

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "class_builder.hpp"
#include "indexable_byte_stream.hpp"
#include "jclassfile/invalid_class_exception.hpp"
#include "jclassfile/io/fallback_instruction_reader.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::io {

class ClassFileReader
{
public:
    ClassFileReader() = default;

    [[nodiscard]] model::ClassFile read(const std::vector<ca::u8>& code);

    // 配置 getter/setter
    [[nodiscard]] bool doDropForwardVersioned() const noexcept { return dropForwardVersioned_; }
    void               setDropForwardVersioned(bool v) noexcept { dropForwardVersioned_ = v; }

    [[nodiscard]] bool doDropBadContextAttributes() const noexcept
    {
        return dropBadContextAttributes_;
    }
    void setDropBadContextAttributes(bool v) noexcept { dropBadContextAttributes_ = v; }

    [[nodiscard]] bool doDropEofAttributes() const noexcept { return dropEofAttributes_; }
    void               setDropEofAttributes(bool v) noexcept { dropEofAttributes_ = v; }

    [[nodiscard]] bool doDropDupeAnnotations() const noexcept { return dropDupeAnnotations_; }
    void               setDropDupeAnnotations(bool v) noexcept { dropDupeAnnotations_ = v; }

    [[nodiscard]] bool doCheckCodeLength() const noexcept { return checkCodeLength_; }
    void               setCheckCodeLength(bool v) noexcept { checkCodeLength_ = v; }

    [[nodiscard]] bool doSkipUnknownPoolTags() const noexcept { return skipUnknownPoolTags_; }
    void               setSkipUnknownPoolTags(bool v) noexcept { skipUnknownPoolTags_ = v; }

    /// @brief 宽容文本解码回调（D40 反馈 bug2）。
    ///
    /// 设置后，常量池 Utf8 条目字节未通过严格 modified-UTF-8 校验时不再抛
    /// InvalidClassException（旧行为会让整个 class 拒读、strings 等消费面静默
    /// 丢类），而是把原始字节交回调规范化为合法 UTF-8（mjt string_extractor
    /// 注入 GBK 恢复，见 shared/text_codec.hpp）。未设置时保持原抛出行为，
    /// 全仓其他消费面零影响。
    using LenientTextDecoder = std::function<std::string(std::string_view)>;
    void setLenientTextDecoder(LenientTextDecoder fn) { lenientTextDecoder_ = std::move(fn); }
    [[nodiscard]] LenientTextDecoder* lenientTextDecoder() noexcept
    {
        return lenientTextDecoder_ ? &lenientTextDecoder_.value() : nullptr;
    }

    // 阶段 5 替换：返回真实的 FallbackInstructionReader。virtual 以支持子类覆盖（如
    // ReservedOpcodeRewritingReader）。
    [[nodiscard]] virtual std::unique_ptr<FallbackInstructionReader> getFallbackInstructionReader(
        ClassBuilder& builder)
    {
        (void)builder;
        return FallbackInstructionReader::fail();
    }

private:
    // 阶段 4b 配置
    bool dropForwardVersioned_     = true;
    bool dropBadContextAttributes_ = true;
    bool dropEofAttributes_        = true;
    bool dropDupeAnnotations_      = true;
    bool checkCodeLength_          = true;
    bool skipUnknownPoolTags_      = false;

    // D40 反馈 bug2：宽容文本解码回调（见 setLenientTextDecoder）。
    std::optional<LenientTextDecoder> lenientTextDecoder_;

    // 内部方法
    [[nodiscard]] std::unique_ptr<model::CpEntry> readPoolEntryBasic(IndexableByteStream& is);
    void readPoolEntryResolve(model::ConstPool& pool, const model::CpEntry* entry,
                              IndexableByteStream& is);
    [[nodiscard]] const model::Field*  readField(ClassBuilder& builder, IndexableByteStream& is);
    [[nodiscard]] const model::Method* readMethod(ClassBuilder& builder, IndexableByteStream& is);
};

}   // namespace jclassfile::io
