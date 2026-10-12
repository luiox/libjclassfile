// 自研：Signature AST Writer 实现，对标 JVMS §4.7.9.1。
// 状态：issue #153 实现

#include "jclassfile/io/signature_writer.hpp"

#include <string>

namespace jclassfile::io {

using namespace model::signature;

namespace {

// ============================================================================
// 类型节点序列化
// ============================================================================

void appendType(std::string& out, const SignatureType& type);

void appendTypeArguments(std::string& out, const std::vector<TypeArgument>& args)
{
    if (args.empty()) return;
    out.push_back('<');
    for (const auto& a : args) {
        switch (a.wildcard()) {
        case WildcardKind::Unbounded: out.push_back('*'); break;
        case WildcardKind::Extends:
            out.push_back('+');
            if (a.type()) appendType(out, *a.type());
            break;
        case WildcardKind::Super:
            out.push_back('-');
            if (a.type()) appendType(out, *a.type());
            break;
        case WildcardKind::Exact:
            if (a.type()) appendType(out, *a.type());
            break;
        }
    }
    out.push_back('>');
}

void appendClassTypeComponent(std::string& out, const std::string& name,
                              const std::vector<TypeArgument>& args)
{
    out.append(name);
    appendTypeArguments(out, args);
}

void appendType(std::string& out, const SignatureType& type)
{
    switch (type.kind()) {
    case SignatureTypeKind::Base:
        out.push_back(static_cast<const BaseType&>(type).descriptor());
        break;
    case SignatureTypeKind::TypeVariable:
        out.push_back('T');
        out.append(static_cast<const TypeVariableType&>(type).name());
        out.push_back(';');
        break;
    case SignatureTypeKind::Array:
    {
        const auto& arr = static_cast<const ArrayType&>(type);
        for (int i = 0; i < arr.dims(); i++) out.push_back('[');
        if (arr.elementType()) appendType(out, *arr.elementType());
        break;
    }
    case SignatureTypeKind::ClassType:
    {
        const auto& ct = static_cast<const ClassType&>(type);
        out.push_back('L');
        appendClassTypeComponent(out, ct.name(), ct.typeArguments());
        for (const auto& inner : ct.innerComponents()) {
            out.push_back('.');
            appendClassTypeComponent(out, inner.name(), inner.typeArguments());
        }
        out.push_back(';');
        break;
    }
    case SignatureTypeKind::Illegal: out.append(static_cast<const IllegalType&>(type).raw()); break;
    }
}

// ============================================================================
// 形式类型参数序列化
// ============================================================================

void appendFormalTypeParameter(std::string& out, const FormalTypeParameter& p)
{
    out.append(p.name());
    out.push_back(':');
    // ClassBound 可为 nullptr（隐式 Object），此时不输出类型
    if (p.classBound()) {
        appendType(out, *p.classBound());
    }
    for (const auto& ib : p.interfaceBounds()) {
        out.push_back(':');
        appendType(out, *ib);
    }
}

void appendTypeParameters(std::string& out, const std::vector<FormalTypeParameter>& params)
{
    if (params.empty()) return;
    out.push_back('<');
    for (const auto& p : params) {
        appendFormalTypeParameter(out, p);
    }
    out.push_back('>');
}

}   // namespace

// ============================================================================
// SignatureWriter::write — 公开入口
// ============================================================================

std::string SignatureWriter::write(const SignatureAst& ast)
{
    std::string out;
    switch (ast.kind()) {
    case SignatureKind::Illegal: return static_cast<const IllegalSignature&>(ast).raw();

    case SignatureKind::Class:
    {
        const auto& cs = static_cast<const ClassSignature&>(ast);
        appendTypeParameters(out, cs.typeParameters());
        if (cs.superclass()) appendType(out, *cs.superclass());
        for (const auto& iface : cs.superinterfaces()) {
            if (iface) appendType(out, *iface);
        }
        break;
    }

    case SignatureKind::Method:
    {
        const auto& ms = static_cast<const MethodSignature&>(ast);
        appendTypeParameters(out, ms.typeParameters());
        out.push_back('(');
        for (const auto& p : ms.parameterTypes()) {
            if (p) appendType(out, *p);
        }
        out.push_back(')');
        if (ms.returnType()) appendType(out, *ms.returnType());
        for (const auto& t : ms.throwsTypes()) {
            out.push_back('^');
            if (t) appendType(out, *t);
        }
        break;
    }

    case SignatureKind::Field:
    {
        const auto& fs = static_cast<const FieldSignature&>(ast);
        if (fs.type()) appendType(out, *fs.type());
        break;
    }
    }
    return out;
}

}   // namespace jclassfile::io
