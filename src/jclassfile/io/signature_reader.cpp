// 自研：Signature AST Reader 实现，对标 JVMS §4.7.9.1。
//
// 容错策略（核心，对齐 cafedude 畸形结构兼容理念）：
//   - read() 永不抛异常。
//   - 顶层无法解析 -> IllegalSignature{raw}。
//   - 局部无法解析 -> 该位置放 IllegalType{rawSubstring}，其余正常节点照常产出，
//     非法子串以语法边界（; > . ( ) < :）截断后保留，保证 round-trip 不丢字符。
//
// 语法（JVMS §4.7.9.1，摘要）：
//   ClassSignature:      FormalTypeParameter* SuperclassSignature SuperinterfaceSignature*
//   MethodSignature:     FormalTypeParameter* ( JavaTypeSignature* ) Result ThrowsSignature*
//   FieldSignature:      ReferenceTypeSignature
//   FormalTypeParameter: Identifier ClassBound InterfaceBound*
//     ClassBound:        ':' [ReferenceTypeSignature]
//     InterfaceBound:    ':' ReferenceTypeSignature
//   ReferenceTypeSignature: ClassTypeSignature | TypeVariableSignature | ArrayTypeSignature
//   JavaTypeSignature:  ReferenceTypeSignature | BaseType
//   ClassTypeSignature: 'L' PackageSpecifier* SimpleClassTypeSignature ClassTypeSignatureSuffix*
//   ';'
//     SimpleClassTypeSignature: Identifier [TypeArguments]
//     TypeArguments: '<' TypeArgument+ '>'
//     TypeArgument: '*' | ('+' | '-')? ReferenceTypeSignature
//   TypeVariableSignature: 'T' Identifier ';'
//   ArrayTypeSignature: ('[')* JavaTypeSignature
//
// 状态：issue #153 实现

#include "jclassfile/io/signature_reader.hpp"

#include "jclassfile/io/signature_writer.hpp"   // 保真校验用

#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace jclassfile::io {

using namespace model::signature;

namespace {

// ============================================================================
// Cursor — 带偏移的游标，解析失败时只回退位置，不抛异常。
// ============================================================================

class Cursor
{
public:
    explicit Cursor(std::string s) noexcept
        : s_(std::move(s))
    {}

    [[nodiscard]] const std::string& str() const noexcept { return s_; }
    [[nodiscard]] size_t             pos() const noexcept { return pos_; }
    [[nodiscard]] bool               eof() const noexcept { return pos_ >= s_.size(); }
    [[nodiscard]] char               peek() const { return eof() ? '\0' : s_[pos_]; }

    char get()
    {
        if (eof()) throw std::out_of_range("cursor eof");
        return s_[pos_++];
    }

    void advance(size_t n = 1) { pos_ += n; }

    /// 回退到指定位置。
    void rewind(size_t p) { pos_ = p; }

    /// 从 from 到当前 pos 的子串。
    [[nodiscard]] std::string substr(size_t from) const { return s_.substr(from, pos_ - from); }

private:
    std::string s_;
    size_t      pos_ = 0;
};

// ============================================================================
// 前向声明
// ============================================================================

std::unique_ptr<SignatureType> parseType(Cursor& cur);
std::unique_ptr<SignatureType> parseReferenceType(Cursor& cur);

/// 读取标识符：JVMS 允许除 . ; [ / : < > 以外的字符构成 identifier。
/// 标识符语法宽松——匹配到下一个边界字符停止。
[[nodiscard]] std::string readIdentifier(Cursor& cur)
{
    size_t start = cur.pos();
    while (!cur.eof()) {
        char c = cur.peek();
        if (c == '.' || c == ';' || c == '[' || c == '/' || c == ':' || c == '<' || c == '>') {
            break;
        }
        cur.advance();
    }
    return cur.substr(start);
}

// ============================================================================
// TypeArguments: '<' TypeArgument+ '>'
// ============================================================================

[[nodiscard]] std::vector<TypeArgument> parseTypeArguments(Cursor& cur)
{
    std::vector<TypeArgument> args;
    // 前置条件：当前已是 '<'
    cur.advance();   // consume '<'
    while (!cur.eof() && cur.peek() != '>') {
        char c = cur.peek();
        if (c == '*') {
            cur.advance();
            args.emplace_back(WildcardKind::Unbounded, nullptr);
        }
        else {
            WildcardKind wk = WildcardKind::Exact;
            if (c == '+') {
                wk = WildcardKind::Extends;
                cur.advance();
            }
            else if (c == '-') {
                wk = WildcardKind::Super;
                cur.advance();
            }
            auto type = parseReferenceType(cur);
            if (!type) {
                // 引用类型解析失败，软失败：本参数吞到下一个 TypeArgument 边界
                break;
            }
            args.emplace_back(wk, std::move(type));
        }
    }
    if (!cur.eof() && cur.peek() == '>') {
        cur.advance();   // consume '>'
    }
    return args;
}

// ============================================================================
// ClassTypeSignature: 'L' SimpleClassTypeSignature ClassTypeSignatureSuffix* ';'
//   SimpleClassTypeSignature: Identifier [TypeArguments]
//   ClassTypeSignatureSuffix: '.' SimpleClassTypeSignature
//
// 注意：name 在 InternalForm 中用 '/' 分隔包名，'.' 分隔内部类层。
// 这里把包名部分（含 '/') 收进最外层 name_，内部类层收集为 innerComponents_。
// ============================================================================

[[nodiscard]] std::unique_ptr<ClassType> parseClassType(Cursor& cur)
{
    // 前置条件：当前已是 'L'
    size_t lpos = cur.pos();
    cur.advance();   // consume 'L'

    // 读取完整类型名直到 ';' '.' '<' ':'，含 '/' 包分隔符。
    // ':' 是 FormalTypeParameter 的 InterfaceBound 分隔符，必须在此停止，
    // 否则 classBound 会贪婪吞掉后续的 ':' 分隔的类型。
    size_t nameStart = cur.pos();
    while (!cur.eof()) {
        char c = cur.peek();
        if (c == ';' || c == '.' || c == '<' || c == ':') {
            break;
        }
        cur.advance();
    }
    std::string outerName = cur.substr(nameStart);

    // 可选 TypeArguments
    std::vector<TypeArgument> outerArgs;
    if (!cur.eof() && cur.peek() == '<') {
        outerArgs = parseTypeArguments(cur);
    }

    auto result = std::make_unique<ClassType>(outerName, std::move(outerArgs));

    // 内部类层：. InnerName [<InnerArgs>]
    std::vector<ClassTypeComponent> components;
    while (!cur.eof() && cur.peek() == '.') {
        cur.advance();   // consume '.'
        size_t innerStart = cur.pos();
        while (!cur.eof()) {
            char c = cur.peek();
            if (c == ';' || c == '.' || c == '<' || c == ':') {
                break;
            }
            cur.advance();
        }
        std::string innerName = cur.substr(innerStart);

        std::vector<TypeArgument> innerArgs;
        if (!cur.eof() && cur.peek() == '<') {
            innerArgs = parseTypeArguments(cur);
        }
        components.emplace_back(innerName, std::move(innerArgs));
    }
    result->setInnerComponents(std::move(components));

    // consume 结尾 ';'
    if (!cur.eof() && cur.peek() == ';') {
        cur.advance();
    }
    (void)lpos;
    return result;
}

// ============================================================================
// ReferenceTypeSignature: ClassTypeSignature | TypeVariableSignature | ArrayTypeSignature
// 失败返回 nullptr（由调用方决定软失败行为）。
// ============================================================================

[[nodiscard]] std::unique_ptr<SignatureType> parseReferenceType(Cursor& cur)
{
    if (cur.eof()) return nullptr;
    char c = cur.peek();
    if (c == 'L') {
        return parseClassType(cur);
    }
    if (c == 'T') {
        size_t start = cur.pos();
        cur.advance();   // consume 'T'
        std::string name = readIdentifier(cur);
        if (cur.eof() || cur.peek() != ';') {
            cur.rewind(start);
            return nullptr;
        }
        cur.advance();   // consume ';'
        return std::make_unique<TypeVariableType>(std::move(name));
    }
    if (c == '[') {
        size_t start = cur.pos();
        int    dims  = 0;
        while (!cur.eof() && cur.peek() == '[') {
            cur.advance();
            dims++;
        }
        // 数组元素可以是任意 JavaTypeSignature（含基本类型）
        if (cur.eof()) {
            cur.rewind(start);
            return nullptr;
        }
        char ec = cur.peek();
        if (ec == 'B' || ec == 'C' || ec == 'D' || ec == 'F' || ec == 'I' || ec == 'J' ||
            ec == 'S' || ec == 'Z') {
            cur.advance();
            auto elem = std::make_unique<BaseType>(ec);
            return std::make_unique<ArrayType>(std::move(elem), dims);
        }
        auto elem = parseReferenceType(cur);
        if (!elem) {
            cur.rewind(start);
            return nullptr;
        }
        return std::make_unique<ArrayType>(std::move(elem), dims);
    }
    return nullptr;
}

// ============================================================================
// JavaTypeSignature: ReferenceTypeSignature | BaseType
// ============================================================================

[[nodiscard]] std::unique_ptr<SignatureType> parseType(Cursor& cur)
{
    if (cur.eof()) return nullptr;
    char c = cur.peek();
    if (c == 'B' || c == 'C' || c == 'D' || c == 'F' || c == 'I' || c == 'J' || c == 'S' ||
        c == 'Z' || c == 'V') {
        cur.advance();
        return std::make_unique<BaseType>(c);
    }
    return parseReferenceType(cur);
}

// ============================================================================
// FormalTypeParameter: Identifier ClassBound InterfaceBound*
//   ClassBound: ':' [ReferenceTypeSignature]
//   InterfaceBound: ':' ReferenceTypeSignature
// ============================================================================

/// 尝试解析一个 FormalTypeParameter。成功返回 true 并填充 out。
/// 失败（无法识别为形式类型参数）返回 false，且不消耗游标（调用方 rewind）。
[[nodiscard]] bool parseFormalTypeParameter(Cursor& cur, FormalTypeParameter& out)
{
    size_t      start = cur.pos();
    std::string name  = readIdentifier(cur);
    if (name.empty() || cur.eof() || cur.peek() != ':') {
        cur.rewind(start);
        return false;
    }
    cur.advance();   // consume ':'

    // ClassBound: ':' [ReferenceTypeSignature]
    // 第一个 ':' 后可能直接跟第二个 ':'（表示 class bound 缺省，即 Object）
    std::unique_ptr<SignatureType> classBound;
    if (!cur.eof() && cur.peek() != ':') {
        classBound = parseReferenceType(cur);
        if (!classBound) {
            cur.rewind(start);
            return false;
        }
    }

    // InterfaceBound*: ':' ReferenceTypeSignature
    std::vector<std::unique_ptr<SignatureType>> interfaceBounds;
    while (!cur.eof() && cur.peek() == ':') {
        cur.advance();   // consume ':'
        auto ib = parseReferenceType(cur);
        if (!ib) {
            cur.rewind(start);
            return false;
        }
        interfaceBounds.push_back(std::move(ib));
    }

    out = FormalTypeParameter(std::move(name), std::move(classBound), std::move(interfaceBounds));
    return true;
}

// ============================================================================
// TypeParameters: '<' FormalTypeParameter+ '>'
// ============================================================================

[[nodiscard]] std::vector<FormalTypeParameter> parseTypeParameters(Cursor& cur)
{
    // 前置条件：当前已是 '<'
    cur.advance();   // consume '<'
    std::vector<FormalTypeParameter> params;
    while (!cur.eof() && cur.peek() != '>') {
        FormalTypeParameter p("", nullptr, {});
        if (!parseFormalTypeParameter(cur, p)) {
            // 无法继续解析形式类型参数，停止
            break;
        }
        params.push_back(std::move(p));
    }
    if (!cur.eof() && cur.peek() == '>') {
        cur.advance();   // consume '>'
    }
    return params;
}

// ============================================================================
// 顶层 ClassSignature: TypeParameters? Superclass Superinterface*
// ============================================================================

[[nodiscard]] std::unique_ptr<SignatureAst> parseClassSignature(Cursor& cur)
{
    std::vector<FormalTypeParameter> typeParams;
    if (!cur.eof() && cur.peek() == '<') {
        typeParams = parseTypeParameters(cur);
    }

    // Superclass
    auto superclass = parseReferenceType(cur);
    if (!superclass) {
        return nullptr;   // 顶层非法，交给 parse() 软失败
    }

    // Superinterface*
    std::vector<std::unique_ptr<SignatureType>> interfaces;
    while (!cur.eof()) {
        auto iface = parseReferenceType(cur);
        if (!iface) break;
        interfaces.push_back(std::move(iface));
    }

    return std::make_unique<ClassSignature>(
        std::move(typeParams), std::move(superclass), std::move(interfaces));
}

// ============================================================================
// 顶层 MethodSignature: TypeParameters? '(' ParamType* ')' Result Throws*
// ============================================================================

[[nodiscard]] std::unique_ptr<SignatureAst> parseMethodSignature(Cursor& cur)
{
    std::vector<FormalTypeParameter> typeParams;
    if (!cur.eof() && cur.peek() == '<') {
        typeParams = parseTypeParameters(cur);
    }

    if (cur.eof() || cur.peek() != '(') {
        return nullptr;
    }
    cur.advance();   // consume '('

    // 参数类型
    std::vector<std::unique_ptr<SignatureType>> params;
    while (!cur.eof() && cur.peek() != ')') {
        auto pt = parseType(cur);
        if (!pt) break;
        params.push_back(std::move(pt));
    }

    if (cur.eof() || cur.peek() != ')') {
        return nullptr;
    }
    cur.advance();   // consume ')'

    // 返回类型（V 也合法）
    auto returnType = parseType(cur);
    if (!returnType) {
        return nullptr;
    }

    // throws: '^' ReferenceTypeSignature*
    std::vector<std::unique_ptr<SignatureType>> throwsTypes;
    while (!cur.eof() && cur.peek() == '^') {
        cur.advance();   // consume '^'
        auto t = parseReferenceType(cur);
        if (!t) break;
        throwsTypes.push_back(std::move(t));
    }

    return std::make_unique<MethodSignature>(
        std::move(typeParams), std::move(params), std::move(returnType), std::move(throwsTypes));
}

// ============================================================================
// 顶层 FieldSignature: ReferenceTypeSignature
// ============================================================================

[[nodiscard]] std::unique_ptr<SignatureAst> parseFieldSignature(Cursor& cur)
{
    auto type = parseReferenceType(cur);
    if (!type) {
        return nullptr;
    }
    return std::make_unique<FieldSignature>(std::move(type));
}

}   // namespace

// ============================================================================
// SignatureReader::read — 公开入口，永不抛异常
// ============================================================================

std::unique_ptr<SignatureAst> SignatureReader::read(std::string signature)
{
    if (signature.empty()) {
        return std::make_unique<IllegalSignature>(std::move(signature));
    }

    Cursor             cur(std::move(signature));
    const std::string& raw = cur.str();

    try {
        std::unique_ptr<SignatureAst> result;

        // 判定 Method：含 '(' 或以 '<' 开头且后续含 '('
        // 判定 Class：以 'L'/'T'/'[' 开头（无 typeParams），或以 '<' 开头且后续不含 '('
        // 判定 Field：以 'L'/'T'/'[' 开头
        char first = raw[0];

        if (first == '(' || first == '<') {
            // '<' 后面可能是 Class 或 Method，用是否含 '(' 区分；
            // 但直接尝试 Method，失败回退 Class。
            size_t mark = cur.pos();
            if (first == '(' || raw.find('(') != std::string::npos) {
                result = parseMethodSignature(cur);
            }
            if (!result) {
                cur.rewind(mark);
                result = parseClassSignature(cur);
            }
        }
        else if (first == 'L' || first == 'T' || first == '[') {
            // 以引用类型开头：可能是 Field（单类型）或 Class（超类 + 接口*）。
            // 先尝试 Field，若未完整消费则回退 Class。
            size_t mark = cur.pos();
            result      = parseFieldSignature(cur);
            if (!result || !cur.eof()) {
                cur.rewind(mark);
                result = parseClassSignature(cur);
            }
        }

        // 任何未能完整消费的解析视为非法（round-trip 会丢字符）
        if (result && !cur.eof()) {
            result.reset();
        }

        // 最终保真校验：read -> write 必须与原串完全一致，
        // 否则视为软失败（对齐 cafedude「round-trip 不丢字符」理念，
        // 也让畸形签名的 best-effort 解析不会静默改字节）。
        if (result) {
            if (SignatureWriter::write(*result) == raw) {
                return result;
            }
            result.reset();
        }
    }
    catch (...) {
        // 所有内部异常都软失败
    }

    return std::make_unique<IllegalSignature>(raw);
}

}   // namespace jclassfile::io
