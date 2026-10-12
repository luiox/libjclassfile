// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/ClassBuilder.java
// 状态：阶段 4b 实现
// 备注：Java builder 收集各部分（pool/version/access/thisClass/superClass/interfaces/
//       fields/methods/attributes），build() 一次性构造成 ClassFile。
//       C++ 镜像 1:1，isAnnotation/isModule 用 CAFED00D Modifiers 公共常量（参见
//       jclassfile::model::ModifierFlag）。
//       thisClass 默认为 placeholder CLASS（与原版一致）；superClass 同。

#pragma once

#include <memory>
#include <type_traits>
#include <vector>

#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/attribute/version_constants.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/placeholders.hpp"
#include "jclassfile/model/field.hpp"
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/method.hpp"

namespace jclassfile::io {

// 与 CAFED00D Modifiers 保持一致的 flag 集合（阶段 4 局部常量）。
namespace modifier_flag {
inline constexpr int ACC_ANNOTATION = 0x2000;
inline constexpr int ACC_MODULE     = 0x8000;
}   // namespace modifier_flag

class ClassBuilder
{
public:
    ClassBuilder()
        : pool_(std::make_unique<model::ConstPool>())
    {
        // 镜像 Java：thisClass/superClass 默认 Placeholders.CLASS
        thisClass_  = model::Placeholders::klass();
        superClass_ = model::Placeholders::klass();
    }

    [[nodiscard]] bool isOakVersion() const noexcept
    {
        return (versionMajor_ == model::VersionConstants::JAVA1 && versionMinor_ <= 2) ||
               (versionMajor_ < model::VersionConstants::JAVA1);
    }

    [[nodiscard]] bool isAnnotation() const noexcept
    {
        return (access_ & modifier_flag::ACC_ANNOTATION) != 0;
    }

    [[nodiscard]] bool isModule() const noexcept
    {
        return (access_ & modifier_flag::ACC_MODULE) != 0;
    }

    [[nodiscard]] const model::ConstPool& getPool() const noexcept { return *pool_; }
    [[nodiscard]] model::ConstPool&       getPool() noexcept { return *pool_; }
    void setConstPool(std::unique_ptr<model::ConstPool> pool) noexcept { pool_ = std::move(pool); }

    [[nodiscard]] int getVersionMajor() const noexcept { return versionMajor_; }
    void              setVersionMajor(int v) noexcept { versionMajor_ = v; }

    [[nodiscard]] int getVersionMinor() const noexcept { return versionMinor_; }
    void              setVersionMinor(int v) noexcept { versionMinor_ = v; }

    [[nodiscard]] int getAccess() const noexcept { return access_; }
    void              setAccess(int a) noexcept { access_ = a; }

    void setThisClass(const model::CpClass* c) noexcept { thisClass_ = c; }
    void setSuperClass(const model::CpClass* c) noexcept { superClass_ = c; }

    [[nodiscard]] const model::CpClass* getThisClass() const noexcept { return thisClass_; }
    [[nodiscard]] const model::CpClass* getSuperClass() const noexcept { return superClass_; }

    void addInterface(const model::CpClass* iface) { interfaces_.push_back(iface); }

    void addField(const model::Field* f) { fields_.push_back(f); }

    void addMethod(const model::Method* m) { methods_.push_back(m); }

    void addAttribute(const model::Attribute* a) { attributes_.push_back(a); }

    /// @brief 在解析 arena 中构造 Attribute，所有权随 build() 转入 ClassFile。
    ///
    /// reader 的所有 attribute 分配必须走此入口（或 createField/createMethod/
    /// adoptInstruction）：解析树节点由 ClassFile 统一拥有并随其析构释放，
    /// 消费方不再需要（也无法）单独释放。
    template<typename T, typename... Args>
    T* createAttribute(Args&&... args)
    {
        static_assert(std::is_base_of_v<model::Attribute, T>, "T must be an Attribute");
        auto owned = std::make_unique<T>(std::forward<Args>(args)...);
        auto raw   = owned.get();
        attributeStorage_.push_back(std::move(owned));
        return raw;
    }

    /// @brief 在解析 arena 中构造 Field（同 createAttribute 的所有权契约）。
    template<typename... Args>
    model::Field* createField(Args&&... args)
    {
        return fieldStorage_.emplace_back(std::make_unique<model::Field>(
                                              std::forward<Args>(args)...))
            .get();
    }

    /// @brief 在解析 arena 中构造 Method（同 createAttribute 的所有权契约）。
    template<typename... Args>
    model::Method* createMethod(Args&&... args)
    {
        return methodStorage_.emplace_back(std::make_unique<model::Method>(
                                               std::forward<Args>(args)...))
            .get();
    }

    /// @brief 接管 InstructionReader 产出的 Instruction 所有权。
    model::Instruction* adoptInstruction(std::unique_ptr<model::Instruction> insn)
    {
        instructionStorage_.push_back(std::move(insn));
        return instructionStorage_.back().get();
    }

    [[nodiscard]] const std::vector<const model::CpClass*>& getInterfaces() const noexcept
    {
        return interfaces_;
    }
    [[nodiscard]] const std::vector<const model::Field*>& getFields() const noexcept
    {
        return fields_;
    }
    [[nodiscard]] const std::vector<const model::Method*>& getMethods() const noexcept
    {
        return methods_;
    }
    [[nodiscard]] const std::vector<const model::Attribute*>& getAttributes() const noexcept
    {
        return attributes_;
    }

    // build() 转移 pool 与全部解析 arena 所有权；fields/methods/attributes 等
    // vector 仍是非拥有视图（指向 arena 内对象），与 ClassFile 的公开访问器
    // 签名保持一致。解析中途抛出时 arena 随 builder 析构释放，无泄漏。
    [[nodiscard]] model::ClassFile build();

private:
    std::unique_ptr<model::ConstPool>    pool_;
    std::vector<const model::CpClass*>   interfaces_;
    std::vector<const model::Field*>     fields_;
    std::vector<const model::Method*>    methods_;
    std::vector<const model::Attribute*> attributes_;
    // 解析 arena：Field/Method/Attribute/Instruction 的唯一所有权所在。
    std::vector<std::unique_ptr<model::Field>>        fieldStorage_;
    std::vector<std::unique_ptr<model::Method>>       methodStorage_;
    std::vector<std::unique_ptr<model::Attribute>>    attributeStorage_;
    std::vector<std::unique_ptr<model::Instruction>>  instructionStorage_;
    int                                  versionMajor_ = 0;
    int                                  versionMinor_ = 0;
    int                                  access_       = 0;
    const model::CpClass*                thisClass_    = nullptr;
    const model::CpClass*                superClass_   = nullptr;
};

}   // namespace jclassfile::io
