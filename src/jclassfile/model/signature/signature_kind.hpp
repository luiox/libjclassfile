// 自研：CAFED00D 原版 SignatureAttribute 仅持有原始 CpUtf8 字符串，不做结构化。
// 本模块为本项目自研的 Signature AST（对标 JVMS §4.7.9.1），对齐 cafedude 的
// 容错理念：畸形签名软失败，产出 Illegal 节点保留原始子串。
// 状态：issue #153 实现

#pragma once

namespace jclassfile::model::signature {

/// 顶层签名 AST 的种类，对标 JVMS §4.7.9.1 的三种签名形式 + Illegal 软失败产物。
enum class SignatureKind
{
    Class,
    Method,
    Field,
    Illegal,
};

/// 签名内类型节点的种类。
/// - Base: 基本类型（I/J/D/F/B/C/S/Z/V）
/// - TypeVariable: 类型变量（TName;）
/// - Array: 数组（[elem）
/// - ClassType: 类类型（Lname<args>;(.inner<args>;)*）
/// - Illegal: 局部解析失败的软失败节点，保留原始子串
enum class SignatureTypeKind
{
    Base,
    TypeVariable,
    Array,
    ClassType,
    Illegal,
};

/// 类型参数的通配符种类，对标 JVMS §4.7.9.1 中 TypeArgument 的 + / - / = / * 四种形式。
/// - Exact: 精确类型（无通配符前缀）
/// - Extends: 上界 ? extends（+）
/// - Super: 下界 ? super（-）
/// - Unbounded: 无界通配符（*，无类型）
enum class WildcardKind
{
    Exact,
    Extends,
    Super,
    Unbounded,
};

}   // namespace jclassfile::model::signature
