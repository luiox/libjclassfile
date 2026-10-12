// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/Descriptor.java

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace jclassfile::model {

class Descriptor
{
public:
    enum class Kind
    {
        Primitive,
        Object,
        Array,
        Method,
        Illegal
    };

    explicit Descriptor(std::string text);

    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    [[nodiscard]] Kind getKind() const noexcept { return kind_; }

    [[nodiscard]] int getArrayLevel() const noexcept { return arrayLevel_; }

    [[nodiscard]] int getParameterSize() const noexcept;

    [[nodiscard]] int getParameterCount() const noexcept;

    [[nodiscard]] std::vector<Descriptor> getParameters() const;

    [[nodiscard]] Descriptor getReturnDesc() const;

    [[nodiscard]] Descriptor getElementDesc() const;

    [[nodiscard]] bool isWide() const noexcept;

    [[nodiscard]] const std::string& getDescriptor() const noexcept { return text_; }

    [[nodiscard]] static Descriptor from(const std::string& desc);
    [[nodiscard]] static Descriptor from(char desc);
    [[nodiscard]] static bool       isPrimitive(const std::string& desc);
    [[nodiscard]] static bool       isPrimitive(char desc);

    static const Descriptor VOID;
    static const Descriptor BOOLEAN;
    static const Descriptor BYTE;
    static const Descriptor CHAR;
    static const Descriptor SHORT;
    static const Descriptor INT;
    static const Descriptor FLOAT;
    static const Descriptor DOUBLE;
    static const Descriptor LONG;
    static const Descriptor OBJECT;

private:
    Descriptor(Kind kind, std::string desc, int arrayLevel = 0)
        : kind_(kind)
        , text_(std::move(desc))
        , arrayLevel_(arrayLevel)
    {}

    // ── 上游 62788a14 游标式解析辅助（私有，供 from(const std::string&) 使用）──

    /// 解析完整方法描述符 (参数)返回值；畸形返回 Kind::Illegal。
    [[nodiscard]] static Descriptor parseMethodDescriptor(const std::string& desc);

    /// 在 cursor 处解析一个字段类型（primitive/object/array），成功推进 cursor；
    /// 'V' 不是字段类型，畸形返回 nullopt。
    [[nodiscard]] static std::optional<Descriptor> parseFieldType(const std::string& desc,
                                                                  std::size_t&       cursor);

    /// 二进制类名（L…; 内部段）不得含 '.'/'['/'<'/'>'。
    [[nodiscard]] static bool isValidBinaryClassName(const std::string& name);

    Kind        kind_;
    std::string text_;
    int         arrayLevel_ = 0;
};

}   // namespace jclassfile::model
