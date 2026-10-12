// 自研：Signature AST 顶层节点，对标 JVMS §4.7.9.1 的三种签名形式。
// CAFED00D 原版 SignatureAttribute 仅持有原始字符串，本模块提供结构化 AST。
// 多态 + kind() 枚举分派，禁用 RTTI；IllegalSignature 为顶层软失败产物。
// 状态：issue #153 实现

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "jclassfile/model/signature/signature_kind.hpp"
#include "jclassfile/model/signature/signature_type.hpp"

namespace jclassfile::model::signature {

/// 顶层签名 AST 的抽象基类。三种具体形式 + Illegal 软失败产物。
class SignatureAst
{
public:
    virtual ~SignatureAst() = default;

    SignatureAst(const SignatureAst&)            = delete;
    SignatureAst& operator=(const SignatureAst&) = delete;

    [[nodiscard]] virtual SignatureKind kind() const noexcept = 0;

protected:
    SignatureAst() = default;
};

// ============================================================================
// ClassSignature: <FormalTypeParameter*> Superclass Superinterface*
// ============================================================================

class ClassSignature : public SignatureAst
{
public:
    ClassSignature(std::vector<FormalTypeParameter>            typeParameters,
                   std::unique_ptr<SignatureType>              superclass,
                   std::vector<std::unique_ptr<SignatureType>> superinterfaces)
        : typeParameters_(std::move(typeParameters))
        , superclass_(std::move(superclass))
        , superinterfaces_(std::move(superinterfaces))
    {}

    [[nodiscard]] SignatureKind kind() const noexcept override { return SignatureKind::Class; }

    [[nodiscard]] const std::vector<FormalTypeParameter>& typeParameters() const noexcept
    {
        return typeParameters_;
    }

    [[nodiscard]] const SignatureType* superclass() const noexcept { return superclass_.get(); }

    [[nodiscard]] const std::vector<std::unique_ptr<SignatureType>>&
    superinterfaces() const noexcept
    {
        return superinterfaces_;
    }

private:
    std::vector<FormalTypeParameter>            typeParameters_;
    std::unique_ptr<SignatureType>              superclass_;
    std::vector<std::unique_ptr<SignatureType>> superinterfaces_;
};

// ============================================================================
// MethodSignature: <FormalTypeParameter*> (ParamType*) ReturnType ThrowsType*
// ============================================================================

class MethodSignature : public SignatureAst
{
public:
    MethodSignature(std::vector<FormalTypeParameter>            typeParameters,
                    std::vector<std::unique_ptr<SignatureType>> parameterTypes,
                    std::unique_ptr<SignatureType>              returnType,
                    std::vector<std::unique_ptr<SignatureType>> throwsTypes)
        : typeParameters_(std::move(typeParameters))
        , parameterTypes_(std::move(parameterTypes))
        , returnType_(std::move(returnType))
        , throwsTypes_(std::move(throwsTypes))
    {}

    [[nodiscard]] SignatureKind kind() const noexcept override { return SignatureKind::Method; }

    [[nodiscard]] const std::vector<FormalTypeParameter>& typeParameters() const noexcept
    {
        return typeParameters_;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<SignatureType>>& parameterTypes() const noexcept
    {
        return parameterTypes_;
    }

    [[nodiscard]] const SignatureType* returnType() const noexcept { return returnType_.get(); }

    [[nodiscard]] const std::vector<std::unique_ptr<SignatureType>>& throwsTypes() const noexcept
    {
        return throwsTypes_;
    }

private:
    std::vector<FormalTypeParameter>            typeParameters_;
    std::vector<std::unique_ptr<SignatureType>> parameterTypes_;
    std::unique_ptr<SignatureType>              returnType_;
    std::vector<std::unique_ptr<SignatureType>> throwsTypes_;
};

// ============================================================================
// FieldSignature: 单个 ReferenceTypeSignature
// ============================================================================

class FieldSignature : public SignatureAst
{
public:
    explicit FieldSignature(std::unique_ptr<SignatureType> type) noexcept
        : type_(std::move(type))
    {}

    [[nodiscard]] SignatureKind kind() const noexcept override { return SignatureKind::Field; }

    [[nodiscard]] const SignatureType* type() const noexcept { return type_.get(); }

private:
    std::unique_ptr<SignatureType> type_;
};

// ============================================================================
// IllegalSignature: 顶层软失败产物，保留原始字符串保证 round-trip。
// ============================================================================

class IllegalSignature : public SignatureAst
{
public:
    explicit IllegalSignature(std::string raw) noexcept
        : raw_(std::move(raw))
    {}

    [[nodiscard]] SignatureKind kind() const noexcept override { return SignatureKind::Illegal; }

    [[nodiscard]] const std::string& raw() const noexcept { return raw_; }

private:
    std::string raw_;
};

}   // namespace jclassfile::model::signature
