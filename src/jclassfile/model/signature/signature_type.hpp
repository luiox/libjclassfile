// 自研：CAFED00D 原版 SignatureAttribute 仅持有原始 CpUtf8 字符串，不做结构化。
// 本文件定义 Signature AST 的类型节点层级，对标 JVMS §4.7.9.1。
// 设计对齐 ElementValue 的多态模式：基类提供 virtual kind() = 0，
// 调用方通过 kind() 枚举做类型分派 + static_cast 还原子类（禁用 RTTI）。
// 容错对齐 Descriptor：局部非法产出 IllegalType 保留原始子串，不中断整棵树。
// 状态：issue #153 实现

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "jclassfile/model/signature/signature_kind.hpp"

namespace jclassfile::model::signature {

class ClassType;

// ============================================================================
// 辅助值类型（非多态）
// ============================================================================

/// 形式类型参数：对标 JVMS §4.7.9.1 的 FormalTypeParameter。
/// 语法：Identifier ClassBound InterfaceBound*
///   ClassBound: ':' [ReferenceTypeSignature]   （classBound_ 为 nullptr 表示缺省上界 Object）
///   InterfaceBound: ':' ReferenceTypeSignature
/// 值类型，可拷贝。
class FormalTypeParameter
{
public:
    FormalTypeParameter(std::string name, std::unique_ptr<class SignatureType> classBound,
                        std::vector<std::unique_ptr<class SignatureType>> interfaceBounds);

    FormalTypeParameter(const FormalTypeParameter&);
    FormalTypeParameter(FormalTypeParameter&&) noexcept = default;
    FormalTypeParameter& operator=(const FormalTypeParameter&);
    FormalTypeParameter& operator=(FormalTypeParameter&&) noexcept = default;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    /// ClassBound，可为 nullptr（表示隐式 java.lang.Object）。
    [[nodiscard]] const SignatureType* classBound() const noexcept { return classBound_.get(); }
    [[nodiscard]] SignatureType*       classBound() noexcept { return classBound_.get(); }

    [[nodiscard]] const std::vector<std::unique_ptr<SignatureType>>&
    interfaceBounds() const noexcept
    {
        return interfaceBounds_;
    }

private:
    std::string                                 name_;
    std::unique_ptr<SignatureType>              classBound_;
    std::vector<std::unique_ptr<SignatureType>> interfaceBounds_;
};

/// 类型参数（泛型调用的实参），对标 JVMS §4.7.9.1 的 TypeArgument。
/// 形式：
///   *                          -> Unbounded（无类型）
///   + ReferenceTypeSignature   -> Extends（上界）
///   - ReferenceTypeSignature   -> Super（下界）
///   ReferenceTypeSignature     -> Exact（精确）
/// Unbounded 时 type_ 为 nullptr；其余三种 type_ 必非空。
/// 值类型，可拷贝。
class TypeArgument
{
public:
    TypeArgument(WildcardKind wildcard, std::unique_ptr<SignatureType> type);

    TypeArgument(const TypeArgument&);
    TypeArgument(TypeArgument&&) noexcept = default;
    TypeArgument& operator=(const TypeArgument&);
    TypeArgument& operator=(TypeArgument&&) noexcept = default;

    [[nodiscard]] WildcardKind wildcard() const noexcept { return wildcard_; }

    /// Unbounded 时返回 nullptr，其余非空。
    [[nodiscard]] const SignatureType* type() const noexcept { return type_.get(); }
    [[nodiscard]] SignatureType*       type() noexcept { return type_.get(); }

private:
    WildcardKind                   wildcard_;
    std::unique_ptr<SignatureType> type_;
};

// ============================================================================
// 类型节点抽象基类
// ============================================================================

/// 签名内类型节点的抽象基类。多态 + kind() 枚举分派，禁用 RTTI。
/// 所有节点持有自身所有权；用 std::unique_ptr 管理。
class SignatureType
{
public:
    virtual ~SignatureType() = default;

    SignatureType(const SignatureType&)            = delete;
    SignatureType& operator=(const SignatureType&) = delete;

    [[nodiscard]] virtual SignatureTypeKind kind() const noexcept = 0;

    /// 深拷贝，用于值类型（FormalTypeParameter/TypeArgument）的拷贝语义。
    [[nodiscard]] virtual std::unique_ptr<SignatureType> clone() const = 0;

protected:
    SignatureType() = default;
};

// ============================================================================
// 具体类型节点
// ============================================================================

/// 基本类型：I/J/D/F/B/C/S/Z/V（单字符 descriptor）。
class BaseType : public SignatureType
{
public:
    explicit BaseType(char descriptor) noexcept
        : descriptor_(descriptor)
    {}

    [[nodiscard]] SignatureTypeKind kind() const noexcept override
    {
        return SignatureTypeKind::Base;
    }
    [[nodiscard]] std::unique_ptr<SignatureType> clone() const override
    {
        return std::make_unique<BaseType>(descriptor_);
    }

    [[nodiscard]] char descriptor() const noexcept { return descriptor_; }

private:
    char descriptor_;
};

/// 类型变量：TName; 引用形式类型参数。
class TypeVariableType : public SignatureType
{
public:
    explicit TypeVariableType(std::string name) noexcept
        : name_(std::move(name))
    {}

    [[nodiscard]] SignatureTypeKind kind() const noexcept override
    {
        return SignatureTypeKind::TypeVariable;
    }
    [[nodiscard]] std::unique_ptr<SignatureType> clone() const override
    {
        return std::make_unique<TypeVariableType>(name_);
    }

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

private:
    std::string name_;
};

/// 数组类型：[elem，dims 为维度数。
class ArrayType : public SignatureType
{
public:
    ArrayType(std::unique_ptr<SignatureType> elementType, int dims) noexcept
        : elementType_(std::move(elementType))
        , dims_(dims)
    {}

    [[nodiscard]] SignatureTypeKind kind() const noexcept override
    {
        return SignatureTypeKind::Array;
    }
    [[nodiscard]] std::unique_ptr<SignatureType> clone() const override;

    [[nodiscard]] const SignatureType* elementType() const noexcept { return elementType_.get(); }
    [[nodiscard]] int                  dims() const noexcept { return dims_; }

private:
    std::unique_ptr<SignatureType> elementType_;
    int                            dims_;
};

/// 类类型的一个组成部分（一个 ClassType 内可能嵌套多个内部类层）。
/// 对标 JVMS §4.7.9.1 的 SimpleClassTypeSignature。
/// 值类型，可拷贝。
class ClassTypeComponent
{
public:
    ClassTypeComponent(std::string name, std::vector<TypeArgument> typeArguments)
        : name_(std::move(name))
        , typeArguments_(std::move(typeArguments))
    {}

    ClassTypeComponent(const ClassTypeComponent&)                = default;
    ClassTypeComponent(ClassTypeComponent&&) noexcept            = default;
    ClassTypeComponent& operator=(const ClassTypeComponent&)     = default;
    ClassTypeComponent& operator=(ClassTypeComponent&&) noexcept = default;

    [[nodiscard]] const std::string&               name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<TypeArgument>& typeArguments() const noexcept
    {
        return typeArguments_;
    }

private:
    std::string               name_;
    std::vector<TypeArgument> typeArguments_;
};

/// 类类型：name<args>;(.innerName<innerArgs>;)*
/// 最外层类名存于 name_，后续内部类层存于 innerComponents_。
class ClassType : public SignatureType
{
public:
    ClassType(std::string name, std::vector<TypeArgument> typeArguments)
        : name_(std::move(name))
        , typeArguments_(std::move(typeArguments))
    {}

    [[nodiscard]] SignatureTypeKind kind() const noexcept override
    {
        return SignatureTypeKind::ClassType;
    }
    [[nodiscard]] std::unique_ptr<SignatureType> clone() const override;

    [[nodiscard]] const std::string&               name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<TypeArgument>& typeArguments() const noexcept
    {
        return typeArguments_;
    }

    /// 内部类层（按源码顺序，空表示无嵌套）。
    [[nodiscard]] const std::vector<ClassTypeComponent>& innerComponents() const noexcept
    {
        return innerComponents_;
    }
    void setInnerComponents(std::vector<ClassTypeComponent> components)
    {
        innerComponents_ = std::move(components);
    }

private:
    std::string                     name_;
    std::vector<TypeArgument>       typeArguments_;
    std::vector<ClassTypeComponent> innerComponents_;
};

/// 软失败节点：保留解析失败处的原始子串，保证 round-trip 不丢字符。
/// 对齐 Descriptor 的 Kind::Illegal 容错理念。
class IllegalType : public SignatureType
{
public:
    explicit IllegalType(std::string raw) noexcept
        : raw_(std::move(raw))
    {}

    [[nodiscard]] SignatureTypeKind kind() const noexcept override
    {
        return SignatureTypeKind::Illegal;
    }
    [[nodiscard]] std::unique_ptr<SignatureType> clone() const override
    {
        return std::make_unique<IllegalType>(raw_);
    }

    [[nodiscard]] const std::string& raw() const noexcept { return raw_; }

private:
    std::string raw_;
};

}   // namespace jclassfile::model::signature
