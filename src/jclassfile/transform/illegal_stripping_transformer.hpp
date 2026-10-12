// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/transform/IllegalStrippingTransformer.java
// 状态：对齐上游 2.6.7（P1 校验族批次，至上游 3c6fc787）。

#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "jclassfile/model/annotation/annotation.hpp"
#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/annotation/target_info.hpp"
#include "jclassfile/model/annotation/type_annotation.hpp"
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/attribute/bootstrap_methods_attribute.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/behavior/attribute_holder.hpp"
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/transform/transformer.hpp"

namespace jclassfile::transform {

class IllegalRewritingInstructionsReader;

class IllegalStrippingTransformer : public Transformer
{
public:
    IllegalStrippingTransformer(model::ClassFile& clazz);

    void transform() override;

    /// 非限定名校验（JVMS 4.2.2）：非空且不含 '.'、';'、'['、'/'。
    /// 上游 f19d7129 新增，AnnotationReader 复用（上游直接调用本类静态方法）。
    [[nodiscard]] static bool isValidUnqualifiedName(std::string_view text);

    /// 内部二进制类名校验：'/' 分段后每段均为非限定名。
    [[nodiscard]] static bool isValidClassName(std::string_view text);

    /// BSM 表压缩后重写所有 dynamic 常量的 bootstrap 索引（上游 7cc58e40 起
    /// 供 AttributeReader 复用；retained[i]=false 表示原第 i 项已被丢弃）。
    static void remapBootstrapMethodIndices(const std::vector<bool>& retained,
                                            const model::ConstPool&  pool);

private:
    // Helpers to find attributes by name (no RTTI)
    static const model::Attribute* findAttributeByName(
        const std::vector<const model::Attribute*>& attrs, std::string_view name);

    static model::Attribute* findAttributeByName(std::vector<const model::Attribute*>& attrs,
                                                 std::string_view                      name);

    // Core stripping methods
    void removeDeadInstructions(model::CodeAttribute& code);
    void removeInvalidInstructions(model::CodeAttribute& code);
    void removeInvalidLineNumbers(model::CodeAttribute& code);
    void removeInvalidVariables(model::CodeAttribute& code);
    void removeInvalidTypeAnnotations(model::CodeAttribute& code);
    void collectDynamicCpReferences(model::CodeAttribute&               code,
                                    std::vector<const model::CpEntry*>& dynamicRefs);
    void removeInvalidBootstrapMethodAttribute(
        const std::vector<const model::CpEntry*>& dynamicRefs);

    // Validation
    bool isValidWrapped(const model::AttributeHolder& holder, const model::Attribute* attribute);
    bool isValid(const model::AttributeHolder& holder, const model::Attribute* attribute);

    // 单例属性去重（上游 baad3a72）：同名 "must only have one of" 属性只保留首个；
    // 递归进入保留的 Code / Record 组件子表（各自有独立的基数规则）。
    void deduplicateAttributes(model::AttributeHolder& holder);

    // Validation accumulators (mirror Java's two Predicate maps keyed by CpEntry
    // identity). expectedTypeMasks: CP entry -> allowed-tag predicate.
    // cpEntryValidators: CP entry -> structural predicate. A null key encodes an
    // unconditional reject (used by type-annotation target checks); a null key
    // whose mask accepts 0 encodes a permitted "index == 0" case (上游 4254753d
    // 起 index==0 语义统一由 mask 自身表达，不再有独立 allow0 开关)。
    using ExpectedTypeMasks = std::unordered_map<const model::CpEntry*, std::function<bool(int)>>;
    using CpEntryValidators =
        std::unordered_map<const model::CpEntry*, std::function<bool(const model::CpEntry*)>>;

    void addAnnotationValidation(const model::AttributeHolder* holder,
                                 ExpectedTypeMasks&            expectedTypeMasks,
                                 CpEntryValidators&            cpEntryValidators,
                                 const model::Annotation*      anno);
    void addElementValueValidation(ExpectedTypeMasks&         expectedTypeMasks,
                                   CpEntryValidators&         cpEntryValidators,
                                   const model::ElementValue* elementValue);

    // BSM / condy 全量校验（上游 fc33b042 / 2c08e5e7 / ed652993 / 7cc58e40）。
    /// active 集合用于防御 dynamic 常量语法级自引用成环（JVMS 4.4.10 允许）。
    [[nodiscard]] bool isValidBootstrapMethod(
        const model::BootstrapMethodsAttribute::BootstrapMethod* bsm,
        std::unordered_set<const model::ConstDynamic*>&          active);
    [[nodiscard]] bool isValidDynamic(const model::ConstDynamic*              dynamic,
                                      std::unordered_set<const model::ConstDynamic*>& active);
    /// 方法句柄 reference kind 与被引用常量类型匹配校验。
    [[nodiscard]] bool isValidMethodHandle(const model::CpMethodHandle* handle) const;
    /// LDC/LDC_W/LDC2_W 操作码与常量种类的 JVMS 6.5 匹配校验（上游 3863965e）。
    [[nodiscard]] static bool isValidLdcEntry(int opcode, const model::CpEntry* entry);
    [[nodiscard]] static bool isWideDynamic(const model::ConstDynamic* dynamic);
    /// two-slot 的 long/double 字段描述符（"J"/"D"，上游 295fae2c）。
    [[nodiscard]] static bool isWideType(const model::CpUtf8* type);

    // Type annotation target 校验（上游 e2e42bb4 补全 + 62af8af2 重构形态）。
    [[nodiscard]] bool isValidTypeAnnotationTarget(const model::AttributeHolder*  holder,
                                                   const model::TypeAnnotation*   typeAnnotation);
    [[nodiscard]] static bool isValidSuperTypeTarget(const model::ClassFile*          classFile,
                                                     const model::SuperTypeTargetInfo* info);
    [[nodiscard]] static bool isValidThrowsTarget(const model::Method*           method,
                                                  const model::ThrowsTargetInfo* info);
    [[nodiscard]] static bool isValidCatchTarget(const model::CodeAttribute*   code,
                                                 const model::CatchTargetInfo* info);

    // Instruction helpers
    static bool isBranch(const model::Instruction* insn);
    static bool isTerminalOrAlwaysTakeFlowControl(const model::Instruction* insn);
    static bool checkInvalidJumpUpTo(const model::CodeAttribute& code, int max);
    static bool checkInvalidJumpFrom(const model::CodeAttribute& code, int min);

    static void shiftJumps(std::vector<const model::Instruction*>& instructions,
                           std::function<bool(int)>                blockRelativeJumpPredicate,
                           std::function<int(int, int)>            rewriter);

    // Stack/locals helpers
    static int computeStack(const model::Instruction* insn);
    static int computeLocals(const model::Instruction* insn);

    /// 调试区间（LVT / type-annotation localvar 目标）是否被 code 长度约束且
    /// 落在指令边界上（上游 1f185aaa）。
    [[nodiscard]] static bool isValidCodeRange(const model::CodeAttribute& code, int codeLength,
                                               int startPc, int length);
    /// offset 是否为某条指令的起始边界。
    [[nodiscard]] static bool isInstructionBoundary(const model::CodeAttribute& code,
                                                    int                         codeLength,
                                                    int                         offset);

    // Match helpers (full validation logic; mirror Java's Predicate factories)
    static bool matchClass(const model::CpEntry* e);
    static bool matchUtf8ValidClassName(const model::CpEntry* e);
    static bool matchUtf8ValidUnqualifiedName(const model::CpEntry* e);
    static bool matchUtf8FieldDescriptor(const model::CpEntry* e);
    static bool matchUtf8MethodDescriptor(const model::CpEntry* e);
    static bool matchUtf8ReturnDescriptor(const model::CpEntry* e);
    static bool matchUtf8NonEmpty(const model::CpEntry* e);
    static bool matchUtf8Word(const model::CpEntry* e);
    static bool matchSignature(const model::CpUtf8* e, const model::AttributeHolder& context);

    /// ConstantValue 属性按字段描述符反查的必须 CP tag（JVMS 4.7.2-A）；
    /// 字段类型不可携带 ConstantValue 时返回 -1（上游 84038ede）。
    [[nodiscard]] static int constantValueTag(const std::string& fieldDesc);

    int reinterpretationPasses_ = 4;
};

}   // namespace jclassfile::transform
