// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/attribute/Attribute.java
// 状态：阶段 3 实现
// 备注：原版 sealed abstract permits 38 个 concrete attribute 子类。
//       C++ 用 abstract class + protected 构造表达 sealed 语义。
//       派生类的"tag"/"isXxx()"机制不在此处约定，由各 concrete 类自行提供。
//       持有 CpUtf8 名称，cpAccesses() 默认返回 {getName()}。

#pragma once

#include <vector>

#include "jclassfile/model/behavior/cp_accessor.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"

namespace jclassfile::model {

enum class AttrKind
{
    Default,
    Code,
    SourceFile,
    LineNumberTable,
    ConstantValue,
    Deprecated,
    Synthetic,
    InnerClasses,
    EnclosingMethod,
    Exceptions,
    Signature,
    BootstrapMethods,
    NestHost,
    NestMembers,
    Record,
    PermittedClasses,
    Module,
    ModulePackages,
    ModuleMainClass,
    ModuleResolution,
    ModuleHashes,
    ModuleTarget,
    SourceDebugExtension,
    CharacterRangeTable,
    StackMapTable,
    LocalVariableTable,
    LocalVariableTypeTable,
    MethodParameters,
    CompilationId,
    SourceId,
    RuntimeVisibleAnnotations,
    RuntimeInvisibleAnnotations,
    RuntimeVisibleParameterAnnotations,
    RuntimeInvisibleParameterAnnotations,
    AnnotationDefault,
};

class Attribute : public behavior::CpAccessor
{
public:
    virtual ~Attribute() = default;

    Attribute(const Attribute&)            = delete;
    Attribute& operator=(const Attribute&) = delete;

    [[nodiscard]] const CpUtf8& name() const noexcept { return *name_; }
    [[nodiscard]] const CpUtf8* namePtr() const noexcept { return name_; }

    [[nodiscard]] virtual int      computeInternalLength() const = 0;
    [[nodiscard]] virtual AttrKind kind() const noexcept { return AttrKind::Default; }

    // marker：替代 Java instanceof CodeAttribute。Method.cpAccesses 需识别
    // native/abstract 方法上的 Code attribute。原版 Java 用 instanceof 配合
    // Modifiers.ACC_NATIVE/ACC_ABSTRACT 跳过其 cpAccesses 收集。
    [[nodiscard]] virtual bool isCodeAttribute() const noexcept { return false; }

    // marker：用于 writer 判断是否为 DefaultAttribute（未知 attribute 回退）
    // 避免 dynamic_cast，符合无 RTTI 约束。
    [[nodiscard]] virtual bool isDefaultAttribute() const noexcept { return false; }

    [[nodiscard]] int computeCompleteLength() const noexcept
    {
        // u2: name_index
        // u4: attribute_length
        // ??: internal length
        return 6 + computeInternalLength();
    }

    [[nodiscard]] std::vector<const CpEntry*> cpAccesses() const override;

protected:
    explicit Attribute(const CpUtf8* name) noexcept
        : name_(name)
    {}

private:
    const CpUtf8* name_;
};

}   // namespace jclassfile::model
