// 自研：Signature AST Writer，对标 JVMS §4.7.9.1。
// 将 Signature AST 序列化回签名字符串（写方向），位于 io 层，
// 与 signature_reader（读方向）对称。model 层只放纯数据模型。
// 配合 reader 实现 round-trip：read -> write 应与原串一致。
// 状态：issue #153 实现

#pragma once

#include <string>

#include "jclassfile/model/signature/signature_ast.hpp"

namespace jclassfile::io {

/// 签名写入器：将 SignatureAst 序列化为 JVMS §4.7.9.1 标准签名字符串。
/// 对标 io 层 AnnotationWriter 的角色（字符串方向）。
/// IllegalSignature 原样输出保留的原始字符串，保证 round-trip 不丢字符。
class SignatureWriter
{
public:
    /// 将 SignatureAst 序列化为标准签名字符串。
    [[nodiscard]] static std::string write(const model::signature::SignatureAst& ast);
};

}   // namespace jclassfile::io
