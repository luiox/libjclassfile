// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/ClassFile.java
// 状态：阶段 4 实现
// 备注：原版 non-sealed class implements Accessible, AttributeHolder, CpAccessor。
//       C++ 三继承 + pool_ 持有（unique_ptr）：
//         - interfaceClasses_ / thisClass_ / superClass_  → const CpClass*（CpEntry 由 ConstPool
//         拥有）
//         - attributes_ → vector<const Attribute*>（与 AttributeHolder 基类签名一致）
//         - fields_     → vector<const Field*>（视图；所有权在 attributeStorage_ 等
//         解析 arena）
//         - methods_    → vector<const Method*>（同上）
//       解析树所有权：ClassFile 经解析 arena（*Storage_ 的 unique_ptr 向量）拥有
//       全部 Field/Method/Attribute/Instruction，随自身析构整树释放；视图向量与
//       公开访问器保持 const 指针签名，消费方不得 delete 任何节点，也不需要。
//       构造允许 superClass=null（与原版 @Nullable 标注一致）。
//       getCp 委托给 ConstPool；cpAccesses() 收集 thisClass + superClass +
//       interfaceClasses + attributes + fields + methods（与原版逐字段对齐）。

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/model/accessible.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/behavior/attribute_holder.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/field.hpp"
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/method.hpp"

namespace jclassfile::model {

class ClassFile : public Accessible, public AttributeHolder, public behavior::CpAccessor
{
public:
    /// 解析树所有权载体：Field/Method/Attribute/Instruction 的 unique_ptr 向量，
    /// 由 ClassBuilder::build() 从解析 arena 移入。视图向量（fields_ 等）指向
    /// arena 内对象。
    struct ModelStorage
    {
        std::vector<std::unique_ptr<Field>>       fields;
        std::vector<std::unique_ptr<Method>>      methods;
        std::vector<std::unique_ptr<Attribute>>   attributes;
        std::vector<std::unique_ptr<Instruction>> instructions;
    };

    ClassFile(int versionMinor, int versionMajor, std::unique_ptr<ConstPool> pool, int access,
              const CpClass* thisClass, const CpClass* superClass,
              std::vector<const CpClass*> interfaceClasses, std::vector<const Field*> fields,
              std::vector<const Method*> methods, std::vector<const Attribute*> attributes,
              ModelStorage storage = {});

    ClassFile(const ClassFile&)                = delete;
    ClassFile& operator=(const ClassFile&)     = delete;
    ClassFile(ClassFile&&) noexcept            = default;
    ClassFile& operator=(ClassFile&&) noexcept = default;

    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::string getSuperName() const;

    [[nodiscard]] const CpEntry* getCp(int index) const;

    [[nodiscard]] const ConstPool& getPool() const noexcept { return *pool_; }
    [[nodiscard]] ConstPool&       getPool() noexcept { return *pool_; }

    [[nodiscard]] const std::vector<const CpClass*>& getInterfaceClasses() const noexcept
    {
        return interfaceClasses_;
    }
    void setInterfaceClasses(std::vector<const CpClass*> interfaceClasses) noexcept
    {
        interfaceClasses_ = std::move(interfaceClasses);
    }

    [[nodiscard]] const std::vector<const Field*>& getFields() const noexcept { return fields_; }
    void setFields(std::vector<const Field*> fields) noexcept { fields_ = std::move(fields); }

    [[nodiscard]] const std::vector<const Method*>& getMethods() const noexcept { return methods_; }
    void setMethods(std::vector<const Method*> methods) noexcept { methods_ = std::move(methods); }

    [[nodiscard]] int getAccess() const noexcept override { return access_; }
    void              setAccess(int access) noexcept override { access_ = access; }

    [[nodiscard]] int getVersionMinor() const noexcept { return versionMinor_; }
    void              setVersionMinor(int versionMinor) noexcept { versionMinor_ = versionMinor; }

    [[nodiscard]] int getVersionMajor() const noexcept { return versionMajor_; }
    void              setVersionMajor(int versionMajor) noexcept { versionMajor_ = versionMajor; }

    [[nodiscard]] const CpClass* getThisClass() const noexcept { return thisClass_; }
    void setThisClass(const CpClass* thisClass) noexcept { thisClass_ = thisClass; }

    [[nodiscard]] const CpClass* getSuperClass() const noexcept { return superClass_; }
    void setSuperClass(const CpClass* superClass) noexcept { superClass_ = superClass; }

    [[nodiscard]] std::vector<const Attribute*>& attributes() noexcept override
    {
        return attributes_;
    }
    [[nodiscard]] const std::vector<const Attribute*>& attributes() const noexcept override
    {
        return attributes_;
    }
    void setAttributes(std::vector<const Attribute*> attributes) noexcept override
    {
        attributes_ = std::move(attributes);
    }

    [[nodiscard]] jclassfile::io::AttributeHolderType holderType() const noexcept override
    {
        return jclassfile::io::AttributeHolderType::CLASS;
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

    /// @brief 解析树节点总数（Field+Method+Attribute+Instruction），用于
    /// 测试与诊断；所有权本身由析构函数处理。
    [[nodiscard]] std::size_t ownedNodeCount() const noexcept
    {
        return storage_.fields.size() + storage_.methods.size() + storage_.attributes.size() +
               storage_.instructions.size();
    }

    /// @brief 接管运行期新建的 Instruction（transformer 改写指令流时产出），
    /// 所有权归入 ModelStorage，与 ClassBuilder::adoptInstruction 契约一致；
    /// 返回裸指针供非拥有视图向量存放。任何绕过本入口的裸 new 指令都会
    /// 随视图析构泄漏。
    model::Instruction* adoptInstruction(std::unique_ptr<model::Instruction> insn)
    {
        storage_.instructions.push_back(std::move(insn));
        return storage_.instructions.back().get();
    }

private:
    std::unique_ptr<ConstPool>    pool_;
    std::vector<const CpClass*>   interfaceClasses_;
    std::vector<const Field*>     fields_;
    std::vector<const Method*>    methods_;
    std::vector<const Attribute*> attributes_;
    ModelStorage                  storage_;
    int                           access_;
    int                           versionMinor_;
    int                           versionMajor_;
    const CpClass*                thisClass_;
    const CpClass*                superClass_;
};

}   // namespace jclassfile::model
