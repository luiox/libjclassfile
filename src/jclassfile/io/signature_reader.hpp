// 自研：Signature AST Reader，对标 JVMS §4.7.9.1。
// CAFED00D 原版 SignatureAttribute 仅持有原始 CpUtf8 字符串，不做结构化。
// 本文件提供签名字符串 -> Signature AST 的解析（读方向），位于 io 层，
// 与 signature_writer（写方向）对称。model 层只放纯数据模型。
// 容错理念：read 永不抛异常；畸形签名软失败为 IllegalSignature。
// 状态：issue #153 实现

#pragma once

#include <memory>
#include <string>

#include "jclassfile/model/signature/signature_ast.hpp"

namespace jclassfile::io {

/// 签名读取器：将签名字符串解析为 AST。
/// 对标 io 层 AnnotationReader 的角色（字符串方向），但输入是签名字符串而非字节流。
/// 容错对齐 Descriptor::from 的 Kind::Illegal 返回模式：read 永不抛异常。
class SignatureReader
{
public:
    /// 将签名字符串解析为 AST。
    /// 顶层根据首字符自动判定 Class/Method/Field 形式（JVMS §4.7.9.1）。
    /// 任何无法解析或 round-trip 不一致的输入都软失败为 IllegalSignature，
    /// 保留原始字符串，永不抛异常。
    /// @param signature 签名字符串（来自 SignatureAttribute 的 CpUtf8 text）。
    /// @return 解析得到的 AST；空串返回 IllegalSignature。
    [[nodiscard]] static std::unique_ptr<model::signature::SignatureAst> read(
        std::string signature);
};

}   // namespace jclassfile::io
