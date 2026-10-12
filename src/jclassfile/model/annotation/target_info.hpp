// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/annotation/TargetInfo.java
// 状态：阶段 6 完整实现
// 备注：
//   - 原版是 abstract + 9 个 public static inner class。
//   - C++ 拆为：抽象基类 TargetInfo + 9 个 final 子类。
//     命名按 dev_plan 2.3 升级为外部类（独立文件 vs. 集中到本文件）。
//     此处为简洁将 9 个子类全部内联到 target_info.hpp / .cpp（避免文件碎片化，
//     与 CodeAttribute::ExceptionTableEntry 已有嵌套风格不同——TargetInfo 子类粒度细
//     且与 TypePath 关系密切，集中便于查阅读；如需拆分可后续按需调整）。
//   - 项目禁用 RTTI：基类提供 virtual TargetInfoKind kind() 枚举做分派。
//   - 字段：targetTypeKind, targetType（与原版逐字一致）。
//   - LocalVarTargetInfo 含内嵌 Variable 类（3 字段：startPc, length, index）。
//   - SuperTypeTargetInfo.EXTENDS = 65535（u2 max）。

#pragma once

#include <cstddef>
#include <vector>

#include "jclassfile/model/annotation/target_info_type.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"

namespace jclassfile::model {

enum class TargetInfoKind
{
    TypeParameter,
    SuperType,
    TypeParameterBound,
    Empty,
    FormalParameter,
    Throws,
    LocalVar,
    Catch,
    Offset,
    TypeArgument,
};

class TargetInfo : public behavior::CpAccessor
{
public:
    virtual ~TargetInfo() = default;

    TargetInfo(const TargetInfo&)            = delete;
    TargetInfo& operator=(const TargetInfo&) = delete;

    [[nodiscard]] virtual TargetInfoKind kind() const noexcept = 0;

    [[nodiscard]] TargetInfoType getTargetTypeKind() const noexcept { return targetTypeKind_; }
    [[nodiscard]] int            getTargetType() const noexcept { return targetType_; }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;
    [[nodiscard]] virtual int                 computeLength() const = 0;

protected:
    TargetInfo(TargetInfoType targetTypeKind, int targetType) noexcept
        : targetTypeKind_(targetTypeKind)
        , targetType_(targetType)
    {}

private:
    TargetInfoType targetTypeKind_;
    int            targetType_;
};

class TypeParameterTargetInfo final : public TargetInfo
{
public:
    TypeParameterTargetInfo(int targetType, int typeParameterIndex) noexcept
        : TargetInfo(TargetInfoType::TYPE_PARAMETER_TARGET, targetType)
        , typeParameterIndex_(typeParameterIndex)
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override
    {
        return TargetInfoKind::TypeParameter;
    }
    [[nodiscard]] int getTypeParameterIndex() const noexcept { return typeParameterIndex_; }
    [[nodiscard]] int computeLength() const noexcept override
    {
        return 1;
    }   // type_parameter_index

private:
    int typeParameterIndex_;
};

class SuperTypeTargetInfo final : public TargetInfo
{
public:
    static constexpr int EXTENDS = 65535;

    SuperTypeTargetInfo(int targetType, int superTypeIndex) noexcept
        : TargetInfo(TargetInfoType::SUPERTYPE_TARGET, targetType)
        , superTypeIndex_(superTypeIndex)
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override
    {
        return TargetInfoKind::SuperType;
    }
    [[nodiscard]] bool isExtends() const noexcept { return superTypeIndex_ == EXTENDS; }
    [[nodiscard]] int  getSuperTypeIndex() const noexcept { return superTypeIndex_; }
    [[nodiscard]] int  computeLength() const noexcept override { return 2; }   // supertype_index

private:
    int superTypeIndex_;
};

class TypeParameterBoundTargetInfo final : public TargetInfo
{
public:
    TypeParameterBoundTargetInfo(int targetType, int typeParameterIndex, int boundIndex) noexcept
        : TargetInfo(TargetInfoType::TYPE_PARAMETER_BOUND_TARGET, targetType)
        , typeParameterIndex_(typeParameterIndex)
        , boundIndex_(boundIndex)
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override
    {
        return TargetInfoKind::TypeParameterBound;
    }
    [[nodiscard]] int getTypeParameterIndex() const noexcept { return typeParameterIndex_; }
    [[nodiscard]] int getBoundIndex() const noexcept { return boundIndex_; }
    [[nodiscard]] int computeLength() const noexcept override
    {
        return 2;
    }   // type_parameter_index + bound_index

private:
    int typeParameterIndex_;
    int boundIndex_;
};

class EmptyTargetInfo final : public TargetInfo
{
public:
    explicit EmptyTargetInfo(int targetType) noexcept
        : TargetInfo(TargetInfoType::EMPTY_TARGET, targetType)
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override { return TargetInfoKind::Empty; }
    [[nodiscard]] int            computeLength() const noexcept override { return 0; }
};

class FormalParameterTargetInfo final : public TargetInfo
{
public:
    FormalParameterTargetInfo(int targetType, int formalParameterIndex) noexcept
        : TargetInfo(TargetInfoType::FORMAL_PARAMETER_TARGET, targetType)
        , formalParameterIndex_(formalParameterIndex)
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override
    {
        return TargetInfoKind::FormalParameter;
    }
    [[nodiscard]] int getFormalParameterIndex() const noexcept { return formalParameterIndex_; }
    [[nodiscard]] int computeLength() const noexcept override
    {
        return 1;
    }   // formal_parameter_index

private:
    int formalParameterIndex_;
};

class ThrowsTargetInfo final : public TargetInfo
{
public:
    ThrowsTargetInfo(int targetType, int throwsTypeIndex) noexcept
        : TargetInfo(TargetInfoType::THROWS_TARGET, targetType)
        , throwsTypeIndex_(throwsTypeIndex)
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override { return TargetInfoKind::Throws; }
    [[nodiscard]] int            getThrowsTypeIndex() const noexcept { return throwsTypeIndex_; }
    [[nodiscard]] int computeLength() const noexcept override { return 2; }   // throws_type_index

private:
    int throwsTypeIndex_;
};

class LocalVarTargetInfo final : public TargetInfo
{
public:
    struct Variable
    {
        int startPc;
        int length;
        int index;

        Variable(int startPc, int length, int index) noexcept
            : startPc(startPc)
            , length(length)
            , index(index)
        {}
    };

    LocalVarTargetInfo(int targetType, std::vector<Variable> variableTable) noexcept
        : TargetInfo(TargetInfoType::LOCALVAR_TARGET, targetType)
        , variableTable_(std::move(variableTable))
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override { return TargetInfoKind::LocalVar; }
    [[nodiscard]] const std::vector<Variable>& getVariableTable() const noexcept
    {
        return variableTable_;
    }
    [[nodiscard]] int computeLength() const noexcept override
    {
        return 2 + (6 * static_cast<int>(variableTable_.size()));
    }

private:
    std::vector<Variable> variableTable_;
};

class CatchTargetInfo final : public TargetInfo
{
public:
    CatchTargetInfo(int targetType, int exceptionTableIndex) noexcept
        : TargetInfo(TargetInfoType::CATCH_TARGET, targetType)
        , exceptionTableIndex_(exceptionTableIndex)
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override { return TargetInfoKind::Catch; }
    [[nodiscard]] int getExceptionTableIndex() const noexcept { return exceptionTableIndex_; }
    [[nodiscard]] int computeLength() const noexcept override
    {
        return 2;
    }   // exception_table_index

private:
    int exceptionTableIndex_;
};

class OffsetTargetInfo final : public TargetInfo
{
public:
    OffsetTargetInfo(int targetType, int offset) noexcept
        : TargetInfo(TargetInfoType::OFFSET_TARGET, targetType)
        , offset_(offset)
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override { return TargetInfoKind::Offset; }
    [[nodiscard]] int            getOffset() const noexcept { return offset_; }
    [[nodiscard]] int            computeLength() const noexcept override { return 2; }   // offset

private:
    int offset_;
};

class TypeArgumentTargetInfo final : public TargetInfo
{
public:
    TypeArgumentTargetInfo(int targetType, int offset, int typeArgumentIndex) noexcept
        : TargetInfo(TargetInfoType::TYPE_ARGUMENT_TARGET, targetType)
        , offset_(offset)
        , typeArgumentIndex_(typeArgumentIndex)
    {}

    [[nodiscard]] TargetInfoKind kind() const noexcept override
    {
        return TargetInfoKind::TypeArgument;
    }
    [[nodiscard]] int getOffset() const noexcept { return offset_; }
    [[nodiscard]] int getTypeArgumentIndex() const noexcept { return typeArgumentIndex_; }
    [[nodiscard]] int computeLength() const noexcept override
    {
        return 3;
    }   // offset + type_argument_index

private:
    int offset_;
    int typeArgumentIndex_;
};

}   // namespace jclassfile::model
