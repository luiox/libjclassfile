// 自研：Signature AST 类型节点实现。对标 JVMS §4.7.9.1。
// 状态：issue #153 实现

#include "jclassfile/model/signature/signature_type.hpp"

namespace jclassfile::model::signature {

// ============================================================================
// 辅助函数：深拷贝 unique_ptr 和 vector<unique_ptr>
// ============================================================================

namespace {

std::unique_ptr<SignatureType> cloneOrNull(const std::unique_ptr<SignatureType>& p)
{
    return p ? p->clone() : nullptr;
}

}   // namespace

// ============================================================================
// FormalTypeParameter
// ============================================================================

FormalTypeParameter::FormalTypeParameter(
    std::string name, std::unique_ptr<SignatureType> classBound,
    std::vector<std::unique_ptr<SignatureType>> interfaceBounds)
    : name_(std::move(name))
    , classBound_(std::move(classBound))
    , interfaceBounds_(std::move(interfaceBounds))
{}

FormalTypeParameter::FormalTypeParameter(const FormalTypeParameter& other)
    : name_(other.name_)
    , classBound_(cloneOrNull(other.classBound_))
{
    interfaceBounds_.reserve(other.interfaceBounds_.size());
    for (const auto& b : other.interfaceBounds_) {
        interfaceBounds_.push_back(cloneOrNull(b));
    }
}

FormalTypeParameter& FormalTypeParameter::operator=(const FormalTypeParameter& other)
{
    if (this != &other) {
        name_       = other.name_;
        classBound_ = cloneOrNull(other.classBound_);
        interfaceBounds_.clear();
        interfaceBounds_.reserve(other.interfaceBounds_.size());
        for (const auto& b : other.interfaceBounds_) {
            interfaceBounds_.push_back(cloneOrNull(b));
        }
    }
    return *this;
}

// ============================================================================
// TypeArgument
// ============================================================================

TypeArgument::TypeArgument(WildcardKind wildcard, std::unique_ptr<SignatureType> type)
    : wildcard_(wildcard)
    , type_(std::move(type))
{}

TypeArgument::TypeArgument(const TypeArgument& other)
    : wildcard_(other.wildcard_)
    , type_(cloneOrNull(other.type_))
{}

TypeArgument& TypeArgument::operator=(const TypeArgument& other)
{
    if (this != &other) {
        wildcard_ = other.wildcard_;
        type_     = cloneOrNull(other.type_);
    }
    return *this;
}

// ============================================================================
// ArrayType
// ============================================================================

std::unique_ptr<SignatureType> ArrayType::clone() const
{
    return std::make_unique<ArrayType>(cloneOrNull(elementType_), dims_);
}

// ============================================================================
// ClassType
// ============================================================================

std::unique_ptr<SignatureType> ClassType::clone() const
{
    auto copy = std::make_unique<ClassType>(name_, typeArguments_);
    copy->setInnerComponents(innerComponents_);
    return copy;
}

}   // namespace jclassfile::model::signature
