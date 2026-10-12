// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/transform/IllegalStrippingTransformer.java
// 状态：对齐上游 2.6.7（P1 校验族批次，至上游 3c6fc787）。

#include "jclassfile/transform/illegal_stripping_transformer.hpp"

#include <algorithm>
#include <cctype>
#include <deque>
#include <stdexcept>
#include <string>

#include "jclassfile/io/annotation_reader.hpp"
#include "jclassfile/io/attribute_reader.hpp"
#include "jclassfile/io/instruction_reader.hpp"
#include "jclassfile/io/instruction_writer.hpp"
#include "jclassfile/model/annotation/annotation_element_value.hpp"
#include "jclassfile/model/annotation/array_element_value.hpp"
#include "jclassfile/model/annotation/class_element_value.hpp"
#include "jclassfile/model/annotation/enum_element_value.hpp"
#include "jclassfile/model/annotation/primitive_element_value.hpp"
#include "jclassfile/model/annotation/target_info.hpp"
#include "jclassfile/model/annotation/type_annotation.hpp"
#include "jclassfile/model/attribute/annotation_default_attribute.hpp"
#include "jclassfile/model/attribute/annotations_attribute.hpp"
#include "jclassfile/model/attribute/attribute_constants.hpp"
#include "jclassfile/model/attribute/attribute_contexts.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/code_utilities.hpp"
#include "jclassfile/model/attribute/local_variable_table_attribute.hpp"
#include "jclassfile/model/attribute/local_variable_type_table_attribute.hpp"
#include "jclassfile/model/attribute/bootstrap_methods_attribute.hpp"
#include "jclassfile/model/attribute/default_attribute.hpp"
#include "jclassfile/model/attribute/constant_value_attribute.hpp"
#include "jclassfile/model/attribute/exceptions_attribute.hpp"
#include "jclassfile/model/attribute/inner_classes_attribute.hpp"
#include "jclassfile/model/attribute/line_number_table_attribute.hpp"
#include "jclassfile/model/attribute/nest_host_attribute.hpp"
#include "jclassfile/model/attribute/nest_members_attribute.hpp"
#include "jclassfile/model/attribute/enclosing_method_attribute.hpp"
#include "jclassfile/model/attribute/parameter_annotations_attribute.hpp"
#include "jclassfile/model/attribute/signature_attribute.hpp"
#include "jclassfile/model/attribute/source_file_attribute.hpp"
#include "jclassfile/model/attribute/module_attribute.hpp"
#include "jclassfile/model/attribute/stack_map_table_attribute.hpp"
#include "jclassfile/model/attribute/permitted_classes_attribute.hpp"
#include "jclassfile/model/attribute/record_attribute.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_method_handle.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"
#include "jclassfile/model/constant/placeholders.hpp"
#include "jclassfile/model/descriptor.hpp"
#include "jclassfile/model/field.hpp"
#include "jclassfile/model/method.hpp"
#include "jclassfile/model/modifiers.hpp"
#include "jclassfile/model/instruction/basic_instruction.hpp"
#include "jclassfile/model/instruction/cp_ref_instruction.hpp"
#include "jclassfile/model/instruction/int_operand_instruction.hpp"
#include "jclassfile/model/instruction/lookup_switch_instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"
#include "jclassfile/model/instruction/table_switch_instruction.hpp"
#include "jclassfile/transform/illegal_rewriting_instructions_reader.hpp"

namespace jclassfile::transform {

using namespace jclassfile::model;
using jclassfile::model::AttributeConstants;

namespace {
constexpr int ACC_ABSTRACT = 0x0400;
constexpr int ACC_NATIVE   = 0x0100;
constexpr int ACC_MODULE   = 0x8000;

// Constant-pool tag values (JVMS §4.4), as ints for the mask predicates that
// mirror Java's numeric comparisons (e.g. INTEGER..STRING).
constexpr int CP_INTEGER       = static_cast<int>(CpTag::INTEGER);
constexpr int CP_FLOAT         = static_cast<int>(CpTag::FLOAT);
constexpr int CP_LONG          = static_cast<int>(CpTag::LONG);
constexpr int CP_DOUBLE        = static_cast<int>(CpTag::DOUBLE);
constexpr int CP_CLASS         = static_cast<int>(CpTag::CLASS);
constexpr int CP_STRING        = static_cast<int>(CpTag::STRING);
constexpr int CP_NAME_TYPE     = static_cast<int>(CpTag::NAME_TYPE);
constexpr int CP_METHOD_HANDLE = static_cast<int>(CpTag::METHOD_HANDLE);
constexpr int CP_DYNAMIC       = static_cast<int>(CpTag::DYNAMIC);
constexpr int CP_UTF8          = static_cast<int>(CpTag::UTF8);
}   // namespace

IllegalStrippingTransformer::IllegalStrippingTransformer(model::ClassFile& clazz)
    : Transformer(clazz)
{}

const Attribute* IllegalStrippingTransformer::findAttributeByName(
    const std::vector<const Attribute*>& attrs, std::string_view name)
{
    for (const Attribute* attr : attrs) {
        if (attr && attr->name().text() == name) return attr;
    }
    return nullptr;
}

Attribute* IllegalStrippingTransformer::findAttributeByName(std::vector<const Attribute*>& attrs,
                                                            std::string_view               name)
{
    for (const Attribute* attr : attrs) {
        if (attr && attr->name().text() == name) return const_cast<Attribute*>(attr);
    }
    return nullptr;
}

void IllegalStrippingTransformer::transform()
{
    // Record existing CP refs (placeholder - actual tracking would need diff logic)
    std::vector<const CpEntry*> cpAccesses;
    // Simplified: we skip the difference-tracking for now

    // Remove members with illegal descriptors.
    // If any of these exist the class isn't going to ever be loaded (without throwing a
    // LinkageError) so its fair game to toss this stuff just to make ASM happy.
    // 字段按字段描述符校验、方法按方法描述符校验（上游 62af8af2）。
    auto& fields = const_cast<std::vector<const Field*>&>(clazz_.getFields());
    fields.erase(
        std::remove_if(fields.begin(), fields.end(), [](const Field* f) {
            return !matchUtf8FieldDescriptor(f->type());
        }),
        fields.end());

    auto& methods = const_cast<std::vector<const Method*>&>(clazz_.getMethods());
    methods.erase(std::remove_if(methods.begin(), methods.end(), [](const Method* m) {
                      return !matchUtf8MethodDescriptor(m->type());
                  }),
                  methods.end());

    // Strip invalid class-level attributes.
    auto& classAttrs = const_cast<std::vector<const Attribute*>&>(clazz_.attributes());
    classAttrs.erase(
        std::remove_if(classAttrs.begin(),
                       classAttrs.end(),
                       [this](const Attribute* a) { return !isValidWrapped(clazz_, a); }),
        classAttrs.end());
    deduplicateAttributes(clazz_);

    for (const Field* field : fields) {
        auto& fieldAttrs = const_cast<std::vector<const Attribute*>&>(field->attributes());
        fieldAttrs.erase(std::remove_if(fieldAttrs.begin(),
                                        fieldAttrs.end(),
                                        [this, field](const Attribute* a) {
                                            return !isValidWrapped(*field, a);
                                        }),
                         fieldAttrs.end());
        deduplicateAttributes(const_cast<Field&>(*field));
    }

    std::vector<const CpEntry*> dynamicCpReferences;

    // Strip invalid method attributes + process code.
    for (const Method* method : methods) {
        auto& methodAttrs = const_cast<std::vector<const Attribute*>&>(method->attributes());
        methodAttrs.erase(std::remove_if(methodAttrs.begin(),
                                         methodAttrs.end(),
                                         [this, method](const Attribute* a) {
                                             return !isValidWrapped(*method, a);
                                         }),
                          methodAttrs.end());
        deduplicateAttributes(const_cast<Method&>(*method));

        // Find Code attribute
        auto* codeAttr = static_cast<CodeAttribute*>(
            const_cast<Attribute*>(findAttributeByName(methodAttrs, AttributeConstants::CODE)));
        if (codeAttr) {
            removeInvalidInstructions(*codeAttr);
            removeInvalidVariables(*codeAttr);
            removeInvalidLineNumbers(*codeAttr);
            removeInvalidTypeAnnotations(*codeAttr);
            collectDynamicCpReferences(*codeAttr, dynamicCpReferences);
        }
    }

    // Strip BSM class attribute if it is unused.
    removeInvalidBootstrapMethodAttribute(dynamicCpReferences);


    // Invalid handles are unusable even when they are otherwise unreferenced
    // （上游 2c08e5e7）。保留引用必然已通过 isValidMethodHandle（BSM ref/args、
    // 未被 stub 的 LDC），因此从池中删除不会留下悬垂引用。
    auto&                        pool          = const_cast<ConstPool&>(pool_);
    std::vector<const CpEntry*> invalidHandles;
    pool.iterator().forEach([&](int, const CpEntry* entry) {
        if (entry->tag() == CpTag::METHOD_HANDLE &&
            !isValidMethodHandle(static_cast<const CpMethodHandle*>(entry)))
            invalidHandles.push_back(entry);
    });
    for (const CpEntry* handle : invalidHandles) pool.remove(handle);
}

void IllegalStrippingTransformer::removeDeadInstructions(CodeAttribute& code)
{
    auto& instructions = const_cast<std::vector<const Instruction*>&>(code.instructions());
    if (instructions.empty()) return;

    std::vector<const Instruction*> visited;
    std::deque<int>                 offsetsToVisit;
    offsetsToVisit.push_back(0);
    for (const auto& e : code.exceptionTable()) {
        offsetsToVisit.push_back(e.handlerPc());
    }

    int insnCount = static_cast<int>(instructions.size());
    while (!offsetsToVisit.empty()) {
        int offset = offsetsToVisit.front();
        offsetsToVisit.pop_front();
        const Instruction* insn = code.getInstructionAtOffset(offset);
        if (!insn) continue;
        int idx = code.indexOf(insn);
        if (idx < 0) continue;
        for (int i = idx; i < insnCount; i++) {
            insn = instructions[i];
            if (std::find(visited.begin(), visited.end(), insn) != visited.end()) break;
            visited.push_back(insn);

            if (isBranch(insn) && insn->kind() == InsnKind::IntOperand) {
                const auto* jump = static_cast<const IntOperandInstruction*>(insn);
                int         base = code.computeOffsetOf(jump);
                offsetsToVisit.push_back(base + jump->getOperand());
            }
            else if (insn->kind() == InsnKind::TableSwitch) {
                const auto* table = static_cast<const TableSwitchInstruction*>(insn);
                int         base  = code.computeOffsetOf(table);
                offsetsToVisit.push_back(base + table->getDefault());
                for (int off : table->getOffsets()) offsetsToVisit.push_back(base + off);
            }
            else if (insn->kind() == InsnKind::LookupSwitch) {
                const auto* lookup = static_cast<const LookupSwitchInstruction*>(insn);
                int         base   = code.computeOffsetOf(lookup);
                offsetsToVisit.push_back(base + lookup->getDefault());
                for (int off : lookup->getOffsets()) offsetsToVisit.push_back(base + off);
            }

            if (isTerminalOrAlwaysTakeFlowControl(insn)) break;
        }
    }

    for (int i = insnCount - 1; i >= 0; i--) {
        Instruction* insn = const_cast<Instruction*>(instructions[i]);
        if (std::find(visited.begin(), visited.end(), insn) == visited.end()) {
            int extraNops   = insn->computeSize() - 1;
            instructions[i] = clazz_.adoptInstruction(
                std::make_unique<BasicInstruction>(Opcodes::NOP));
            for (int j = 0; j < extraNops; j++) {
                instructions.insert(
                    instructions.begin() + i,
                    clazz_.adoptInstruction(std::make_unique<BasicInstruction>(Opcodes::NOP)));
            }
        }
    }
}

void IllegalStrippingTransformer::removeInvalidInstructions(CodeAttribute& code)
{
    auto& instructions = const_cast<std::vector<const Instruction*>&>(code.instructions());

    // CAFED00D 1b1af8bc：junk try-catch 剪枝必须在 size()<=1 早退之前——
    // 单指令方法同样可能残留越界/空区间条目。
    int maxPc = instructions.empty() ? 0 : code.computeOffsetOf(instructions.back());
    auto& exceptions =
        const_cast<std::vector<CodeAttribute::ExceptionTableEntry>&>(code.exceptionTable());
    for (int i = static_cast<int>(exceptions.size()) - 1; i >= 0; i--) {
        const auto& entry = exceptions[i];
        if (entry.startPc() > maxPc || entry.endPc() > maxPc || entry.handlerPc() > maxPc ||
            entry.startPc() >= entry.endPc())
            exceptions.erase(exceptions.begin() + i);
    }

    if (instructions.size() <= 1) return;

    // Remove invalid jumps and LDC instructions
    for (int i = static_cast<int>(instructions.size()) - 1; i >= 0; i--) {
        Instruction* insn = const_cast<Instruction*>(instructions[i]);
        int          op   = insn->getOpcode();

        // Jumps that go out of bounds of the method
        if (isBranch(insn) && insn->kind() == InsnKind::IntOperand) {
            auto* jump       = static_cast<IntOperandInstruction*>(insn);
            int   jumpTarget = code.computeOffsetOf(jump) + jump->getOperand();
            if (jumpTarget > maxPc || jumpTarget < 0) {
                int size        = insn->computeSize();
                instructions[i] = clazz_.adoptInstruction(
                    std::make_unique<BasicInstruction>(Opcodes::RETURN));
                for (int j = 0; j < size - 1; j++)
                    instructions.insert(
                        instructions.begin() + i,
                        clazz_.adoptInstruction(std::make_unique<BasicInstruction>(Opcodes::NOP)));
            }
        }
        else if (insn->kind() == InsnKind::TableSwitch) {
            auto* tswitch = static_cast<TableSwitchInstruction*>(insn);
            int   base    = code.computeOffsetOf(tswitch);
            bool  bad = (base + tswitch->getDefault() > maxPc || base + tswitch->getDefault() < 0);
            if (!bad) {
                for (int off : tswitch->getOffsets()) {
                    if (base + off > maxPc || base + off < 0) {
                        bad = true;
                        break;
                    }
                }
            }
            if (bad) {
                int size        = insn->computeSize();
                instructions[i] = clazz_.adoptInstruction(
                    std::make_unique<BasicInstruction>(Opcodes::RETURN));
                for (int j = 0; j < size - 1; j++)
                    instructions.insert(
                        instructions.begin() + i,
                        clazz_.adoptInstruction(std::make_unique<BasicInstruction>(Opcodes::NOP)));
            }
        }
        else if (insn->kind() == InsnKind::LookupSwitch) {
            auto* lswitch = static_cast<LookupSwitchInstruction*>(insn);
            int   base    = code.computeOffsetOf(lswitch);
            bool  bad = (base + lswitch->getDefault() > maxPc || base + lswitch->getDefault() < 0);
            if (!bad) {
                for (int off : lswitch->getOffsets()) {
                    if (base + off > maxPc || base + off < 0) {
                        bad = true;
                        break;
                    }
                }
            }
            if (bad) {
                int size        = insn->computeSize();
                instructions[i] = clazz_.adoptInstruction(
                    std::make_unique<BasicInstruction>(Opcodes::RETURN));
                for (int j = 0; j < size - 1; j++)
                    instructions.insert(
                        instructions.begin() + i,
                        clazz_.adoptInstruction(std::make_unique<BasicInstruction>(Opcodes::NOP)));
            }
        }
        // Dynamic constants must point to an existing, valid bootstrap method
        // （上游 fc33b042 / 2c08e5e7）。
        else if (insn->kind() == InsnKind::CpRef) {
            auto*          ldc   = static_cast<CpRefInstruction*>(insn);
            const CpEntry* entry = ldc->getEntry();
            bool           stub  = false;
            if (entry && entry->isConstDynamic()) {
                std::unordered_set<const ConstDynamic*> active;
                stub = !isValidDynamic(static_cast<const ConstDynamic*>(entry), active);
            }
            // LDC content must match the opcode's constant-kind rules and method
            // handles must match their reference kind（上游 3863965e / fc33b042）。
            else if (op == Opcodes::LDC || op == Opcodes::LDC_W || op == Opcodes::LDC2_W) {
                bool handleMismatch =
                    entry != nullptr && entry->tag() == CpTag::METHOD_HANDLE &&
                    !isValidMethodHandle(static_cast<const CpMethodHandle*>(entry));
                stub = !isValidLdcEntry(op, entry) || handleMismatch;
            }
            if (stub) {
                int size        = insn->computeSize();
                instructions[i] = clazz_.adoptInstruction(
                    std::make_unique<BasicInstruction>(Opcodes::RETURN));
                for (int j = 0; j < size - 1; j++)
                    instructions.insert(
                        instructions.begin() + i,
                        clazz_.adoptInstruction(std::make_unique<BasicInstruction>(Opcodes::NOP)));
            }
        }
    }
}

void IllegalStrippingTransformer::removeInvalidLineNumbers(CodeAttribute& code)
{
    auto& attrs = const_cast<std::vector<const Attribute*>&>(code.attributes());
    auto* lines = static_cast<LineNumberTableAttribute*>(const_cast<Attribute*>(
        findAttributeByName(attrs, AttributeConstants::LINE_NUMBER_TABLE)));
    if (lines == nullptr) return;

    // 空指令序列的 Code（敌意输入）：computeSize 内部 back() 对空向量是 UB，
    // 视作 codeLength=0 → 全部条目越界剥离。
    int codeLength = code.instructions().empty() ? 0 : code.computeSize();
    auto& entries =
        const_cast<std::vector<LineNumberTableAttribute::LineEntry>&>(lines->entries());
    entries.erase(std::remove_if(entries.begin(),
                                 entries.end(),
                                 [&](const LineNumberTableAttribute::LineEntry& entry) {
                                     return !isInstructionBoundary(code, codeLength,
                                                                   entry.startPc());
                                 }),
                  entries.end());
}

void IllegalStrippingTransformer::removeInvalidVariables(CodeAttribute& code)
{
    // In ASM's ClassReader.readCode(...) the startPc + length are used as labels[pc+length]
    // which can be OOB.
    // 同上：空指令序列时按 codeLength=0 处理，避免 computeSize 的空向量 UB。
    int codeLength = code.instructions().empty() ? 0 : code.computeSize();
    int maxLocals  = code.maxLocals();

    // Strip invalid entries from the local variable table.
    //  - The range must be bounded by the code length.
    //  - The local variable index must be within the max locals of the method.
    //  - A long/double occupies two slots, so its second slot must also be in range
    //    （上游 295fae2c：表尾无 padding 位 的 wide 类型此前漏清理）。
    auto& attrs = const_cast<std::vector<const Attribute*>&>(code.attributes());

    auto* lvtAttr = static_cast<LocalVariableTableAttribute*>(const_cast<Attribute*>(
        findAttributeByName(attrs, AttributeConstants::LOCAL_VARIABLE_TABLE)));
    if (lvtAttr) {
        auto& entries =
            const_cast<std::vector<LocalVariableTableAttribute::VarEntry>&>(lvtAttr->entries());
        entries.erase(
            std::remove_if(entries.begin(),
                           entries.end(),
                           [&](const LocalVariableTableAttribute::VarEntry& e) {
                               return !isValidCodeRange(code, codeLength, e.startPc(), e.length()) ||
                                      e.index() >= maxLocals ||
                                      (isWideType(e.desc()) && e.index() + 1 >= maxLocals);
                           }),
            entries.end());
    }

    // Same rules apply to the local variable type table.
    auto* lvttAttr = static_cast<LocalVariableTypeTableAttribute*>(const_cast<Attribute*>(
        findAttributeByName(attrs, AttributeConstants::LOCAL_VARIABLE_TYPE_TABLE)));
    if (lvttAttr) {
        auto& entries = const_cast<std::vector<LocalVariableTypeTableAttribute::VarTypeEntry>&>(
            lvttAttr->entries());
        entries.erase(
            std::remove_if(entries.begin(),
                           entries.end(),
                           [&](const LocalVariableTypeTableAttribute::VarTypeEntry& e) {
                               return !isValidCodeRange(code, codeLength, e.startPc(), e.length()) ||
                                      e.index() >= maxLocals ||
                                      (isWideType(e.signature()) && e.index() + 1 >= maxLocals);
                           }),
            entries.end());
    }
}

void IllegalStrippingTransformer::removeInvalidTypeAnnotations(CodeAttribute& code)
{
    // 同上：空指令序列时按 codeLength=0 处理，避免 computeSize 的空向量 UB。
    int  codeLength = code.instructions().empty() ? 0 : code.computeSize();
    auto& attributes = const_cast<std::vector<const Attribute*>&>(code.attributes());
    for (int i = static_cast<int>(attributes.size()) - 1; i >= 0; i--) {
        // Only care about type annotations.
        const Attribute*   attribute = attributes[i];
        const std::string& name      = attribute->name().text();
        if (name != AttributeConstants::RUNTIME_INVISIBLE_TYPE_ANNOTATIONS &&
            name != AttributeConstants::RUNTIME_VISIBLE_TYPE_ANNOTATIONS)
            continue;
        auto* annotations = static_cast<AnnotationsAttribute*>(const_cast<Attribute*>(attribute));

        // 上游 1f185aaa / d634db37：
        //  - localvar 目标复用 LVT 的区间规则（长度受 code 约束 + 槽位 < maxLocals）；
        //  - offset / type-argument 目标必须对齐指令边界（javac 记录的是前置取数
        //    指令的 offset，因此只查边界、不查操作码）。
        auto& list =
            const_cast<std::vector<std::unique_ptr<Annotation>>&>(annotations->annotations());
        list.erase(std::remove_if(list.begin(),
                                  list.end(),
                                  [&](const std::unique_ptr<Annotation>& annotation) {
                                      // Skip non-type annotations.
                                      if (annotation->kind() != AnnotationKind::TypeAnnotation)
                                          return false;
                                      const auto* typeAnnotation =
                                          static_cast<const TypeAnnotation*>(annotation.get());
                                      const TargetInfo* targetInfo =
                                          typeAnnotation->getTargetInfo();
                                      if (targetInfo == nullptr) return false;

                                      // Local-variable targets duplicate the range rules
                                      // of LocalVariableTable.
                                      if (targetInfo->getTargetTypeKind() ==
                                          TargetInfoType::LOCALVAR_TARGET) {
                                          const auto* localVar =
                                              static_cast<const LocalVarTargetInfo*>(targetInfo);
                                          for (const auto& variable : localVar->getVariableTable()) {
                                              if (!isValidCodeRange(code, codeLength,
                                                                    variable.startPc,
                                                                    variable.length) ||
                                                  variable.index >= code.maxLocals()) {
                                                  return true;
                                              }
                                          }
                                      }

                                      // Offset targets must be aligned to instruction
                                      // boundaries.
                                      if (targetInfo->getTargetTypeKind() ==
                                          TargetInfoType::OFFSET_TARGET) {
                                          const auto* offsetTarget =
                                              static_cast<const OffsetTargetInfo*>(targetInfo);
                                          return !isInstructionBoundary(code, codeLength,
                                                                        offsetTarget->getOffset());
                                      }

                                      // Type-argument targets must be aligned to
                                      // instruction boundaries.
                                      if (targetInfo->getTargetTypeKind() ==
                                          TargetInfoType::TYPE_ARGUMENT_TARGET) {
                                          const auto* typeArgumentTarget =
                                              static_cast<const TypeArgumentTargetInfo*>(targetInfo);
                                          return !isInstructionBoundary(code, codeLength,
                                                                        typeArgumentTarget->getOffset());
                                      }

                                      return false;
                                  }),
                  list.end());

        // Drop the attribute if it has no annotations left.
        if (list.empty()) attributes.erase(attributes.begin() + i);
    }
}

void IllegalStrippingTransformer::collectDynamicCpReferences(
    CodeAttribute& code, std::vector<const CpEntry*>& dynamicRefs)
{
    for (const Instruction* insn : code.instructions()) {
        if (insn->kind() == InsnKind::CpRef) {
            const auto*    cpRef = static_cast<const CpRefInstruction*>(insn);
            const CpEntry* entry = cpRef->getEntry();
            if (entry && entry->isConstDynamic()) {
                if (std::find(dynamicRefs.begin(), dynamicRefs.end(), entry) == dynamicRefs.end())
                    dynamicRefs.push_back(entry);
            }
        }
    }
}

bool IllegalStrippingTransformer::isValidBootstrapMethod(
    const BootstrapMethodsAttribute::BootstrapMethod*   bsm,
    std::unordered_set<const ConstDynamic*>&            active)
{
    if (bsm == nullptr || bsm->bsmMethodRef() == nullptr ||
        !isValidMethodHandle(bsm->bsmMethodRef()))
        return false;

    for (const CpEntry* arg : bsm->args()) {
        // Bootstrap arguments must be constants that the VM can load.
        // JVMS 4.4-C:
        //  - 3:  CONSTANT_Integer
        //  - 4:  CONSTANT_Float
        //  - 5:  CONSTANT_Long
        //  - 6:  CONSTANT_Double
        //  - 7:  CONSTANT_Class
        //  - 8:  CONSTANT_String
        //  - 15: CONSTANT_MethodHandle
        //  - 16: CONSTANT_MethodType
        //  - 17: CONSTANT_Dynamic
        if (arg == nullptr) return false;
        int tag = static_cast<int>(arg->tag());
        if (!((tag >= CP_INTEGER && tag <= CP_STRING) ||
              (tag >= CP_METHOD_HANDLE && tag <= CP_DYNAMIC)))
            return false;

        // Method handles and dynamic constants have additional bootstrap-specific
        // validity rules.
        if (arg->tag() == CpTag::METHOD_HANDLE &&
            !isValidMethodHandle(static_cast<const CpMethodHandle*>(arg)))
            return false;
        if (arg->isConstDynamic() &&
            !isValidDynamic(static_cast<const ConstDynamic*>(arg), active))
            return false;
    }
    return true;
}

void IllegalStrippingTransformer::remapBootstrapMethodIndices(const std::vector<bool>& retained,
                                                              const ConstPool&         pool)
{
    // Build an old-index to new-index map, using -1 for entries that no longer exist.
    std::vector<int> indexMap(retained.size());
    int              nextIndex = 0;
    for (std::size_t i = 0; i < retained.size(); i++)
        indexMap[i] = retained[i] ? nextIndex++ : -1;

    // Rewrite every dynamic pool entry before bytecode validation checks its bootstrap
    // target. C++ bsmIndex 为 u16，"不存在"与上游 Java 的 -1 同写为 0xFFFF
    // （越界 → 后续 isValidDynamic 边界检查失败），字节输出与上游一致。
    auto& mutablePool = const_cast<ConstPool&>(pool);
    mutablePool.iterator().forEach([&](int, const CpEntry* entry) {
        if (!entry->isConstDynamic()) return;
        auto* dynamic  = const_cast<ConstDynamic*>(static_cast<const ConstDynamic*>(entry));
        int   oldIndex = static_cast<int>(dynamic->bsmIndex());
        int   newIndex = (oldIndex >= 0 && static_cast<std::size_t>(oldIndex) < indexMap.size())
                             ? indexMap[static_cast<std::size_t>(oldIndex)]
                             : -1;
        dynamic->setBsmIndex(static_cast<ca::u16>(newIndex));
    });
}

void IllegalStrippingTransformer::removeInvalidBootstrapMethodAttribute(
    const std::vector<const CpEntry*>& dynamicRefs)
{
    // ASM will try to read this attribute if any CP entry exists for DYNAMIC or
    // INVOKE_DYNAMIC. If no methods actually refer to those CP entries, this attribute
    // can be filled with garbage, in which case we will want to remove it.
    auto& classAttrs = const_cast<std::vector<const Attribute*>&>(clazz_.attributes());
    if (dynamicRefs.empty()) {
        // There are no dynamic references found in any method's code. Remove the BSM
        // attribute.
        auto* bsmAttr = const_cast<Attribute*>(
            findAttributeByName(classAttrs, AttributeConstants::BOOTSTRAP_METHODS));
        if (bsmAttr) {
            classAttrs.erase(std::remove(classAttrs.begin(), classAttrs.end(), bsmAttr),
                             classAttrs.end());
        }
    }

    // Remove any unused dynamic cp references in the constant pool.
    // C++ 与上游的一处保守偏差：池条目所有权归 ConstPool，remove 即销毁对象。
    // 仍被保留 BSM 条目的参数引用的 dynamic 若从池中移除会留下悬垂指针（上游
    // Java 靠 GC 天然无此问题），此处跳过这类条目不删，宁可保守不误伤。
    const Attribute* bsmAttr =
        findAttributeByName(classAttrs, AttributeConstants::BOOTSTRAP_METHODS);
    std::vector<const CpEntry*> bsmArgDynamics;
    if (bsmAttr != nullptr) {
        const auto* bsmAttribute = static_cast<const BootstrapMethodsAttribute*>(bsmAttr);
        for (const auto& bsm : bsmAttribute->bootstrapMethods()) {
            for (const CpEntry* arg : bsm.args()) {
                if (arg != nullptr && arg->isConstDynamic()) bsmArgDynamics.push_back(arg);
            }
        }
    }

    auto&                        pool = const_cast<ConstPool&>(pool_);
    std::vector<const CpEntry*> toRemove;
    pool.iterator().forEach([&](int, const CpEntry* entry) {
        if (!entry->isConstDynamic()) return;
        if (std::find(dynamicRefs.begin(), dynamicRefs.end(), entry) != dynamicRefs.end()) return;
        if (std::find(bsmArgDynamics.begin(), bsmArgDynamics.end(), entry) !=
            bsmArgDynamics.end())
            return;
        toRemove.push_back(entry);
    });
    for (const CpEntry* entry : toRemove) pool.remove(entry);
}

bool IllegalStrippingTransformer::isValidWrapped(const AttributeHolder& holder,
                                                 const Attribute*       attribute)
{
    try {
        return isValid(holder, attribute);
    }
    catch (...) {
        // Any parse/cast failure while inspecting the attribute => drop it.
        return false;
    }
}

bool IllegalStrippingTransformer::isValid(const AttributeHolder& holder, const Attribute* attribute)
{
    ExpectedTypeMasks expectedTypeMasks;
    CpEntryValidators cpEntryValidators;

    // Check name index is within pool range.
    const std::size_t maxCpIndex = pool_.size();
    if (attribute->name().index() > maxCpIndex) return false;

    // Cannot investigate directly unsupported attributes.
    if (attribute->isDefaultAttribute()) return true;

    const std::string& name = attribute->name().text();

    // Check for illegal usage contexts.
    auto                    allowedContexts = AttributeContexts::getAllowedContexts(name);
    io::AttributeHolderType holderType      = holder.holderType();
    if (std::find(allowedContexts.begin(), allowedContexts.end(), holderType) ==
        allowedContexts.end()) {
        return false;
    }
    else if (!Modifiers::has(static_cast<uint16_t>(clazz_.getAccess()), Modifiers::ACC_MODULE) &&
             io::AttributeReader::isModuleAttribute(name)) {
        // module* attributes only legal on module classes（上游 fd36c0a7：按名
        // 集中校验，取代散落的名称前缀特判）。
        return false;
    }

    // Check indices match certain types (key=cp_entry, value=allowed-tag predicate).
    // index==0（模型中的 nullptr 引用）是否合法由 mask 自身能否接受 0 决定
    // （上游 4254753d）。
    if (name == AttributeConstants::CONSTANT_VALUE) {
        const auto* attr = static_cast<const ConstantValueAttribute*>(attribute);
        // ConstantValue 只保留 JVMS 4.7.2-A 列出的、与字段类型匹配的常量类型
        // （上游 84038ede）。
        const auto* field       = static_cast<const Field*>(&holder);
        int         requiredTag = constantValueTag(field->type()->text());
        if (requiredTag < 0) return false;
        const CpEntry* valueIndex           = attr->constantValue();
        expectedTypeMasks[valueIndex]       = [requiredTag](int i) { return i == requiredTag; };
    }
    else if (name == AttributeConstants::RUNTIME_INVISIBLE_ANNOTATIONS ||
             name == AttributeConstants::RUNTIME_VISIBLE_ANNOTATIONS ||
             name == AttributeConstants::RUNTIME_INVISIBLE_TYPE_ANNOTATIONS ||
             name == AttributeConstants::RUNTIME_VISIBLE_TYPE_ANNOTATIONS) {
        const auto* attr = static_cast<const AnnotationsAttribute*>(attribute);
        for (const auto& anno : attr->annotations())
            addAnnotationValidation(&holder, expectedTypeMasks, cpEntryValidators, anno.get());
    }
    else if (name == AttributeConstants::RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS ||
             name == AttributeConstants::RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS) {
        // Sanity check
        if (holderType != io::AttributeHolderType::METHOD) return false;
        const auto* attr = static_cast<const ParameterAnnotationsAttribute*>(attribute);

        // Compare against actual number of parameters. The C++ model stores
        // parameter annotations as a dense vector-of-lists, so an out-of-bounds
        // index manifests as more lists than the method has parameters.
        const auto* method         = static_cast<const Method*>(&holder);
        Descriptor  desc           = Descriptor::from(method->type()->text());
        int         parameterCount = desc.getParameterCount();
        if (static_cast<int>(attr->parameterAnnotations().size()) > parameterCount) return false;

        for (const auto& annotationList : attr->parameterAnnotations())
            for (const auto& anno : annotationList)
                addAnnotationValidation(&holder, expectedTypeMasks, cpEntryValidators, anno.get());
    }
    else if (name == AttributeConstants::ANNOTATION_DEFAULT) {
        // AnnotationDefault 只允许出现在注解接口（ACC_ANNOTATION）的方法上
        // （上游 823233cf）。
        if (!Modifiers::has(static_cast<uint16_t>(clazz_.getAccess()), Modifiers::ACC_ANNOTATION))
            return false;
        const auto* attr = static_cast<const AnnotationDefaultAttribute*>(attribute);
        addElementValueValidation(expectedTypeMasks, cpEntryValidators, attr->elementValue());
    }
    else if (name == AttributeConstants::NEST_HOST) {
        const auto* attr                     = static_cast<const NestHostAttribute*>(attribute);
        cpEntryValidators[attr->hostClass()] = &IllegalStrippingTransformer::matchClass;
    }
    else if (name == AttributeConstants::NEST_MEMBERS) {
        const auto* attr = static_cast<const NestMembersAttribute*>(attribute);
        for (const CpClass* member : attr->memberClasses())
            cpEntryValidators[member] = &IllegalStrippingTransformer::matchClass;
    }
    else if (name == AttributeConstants::ENCLOSING_METHOD) {
        const auto* attr = static_cast<const EnclosingMethodAttribute*>(attribute);
        expectedTypeMasks[attr->classEntry()] = [](int i) { return i == CP_CLASS; };
        cpEntryValidators[attr->classEntry()] = &IllegalStrippingTransformer::matchClass;
        // method_index must be zero if enclosed by an initializer; else a NameType.
        // 0 的合法性由 mask 接受 0 表达（上游 4254753d）。
        expectedTypeMasks[attr->methodEntry()] = [](int i) {
            return i == 0 || i == CP_NAME_TYPE;
        };
    }
    else if (name == AttributeConstants::EXCEPTIONS) {
        const auto* attr = static_cast<const ExceptionsAttribute*>(attribute);
        for (const CpClass* exceptionType : attr->exceptionTable())
            cpEntryValidators[exceptionType] = &IllegalStrippingTransformer::matchClass;
    }
    else if (name == AttributeConstants::INNER_CLASSES) {
        const auto* attr = static_cast<const InnerClassesAttribute*>(attribute);
        for (const auto& innerClass : attr->innerClasses()) {
            // Must be a valid nonzero CONSTANT_Class identifying the class being
            // described.
            expectedTypeMasks[innerClass.innerClassInfo()] = [](int i) {
                return i == CP_CLASS;
            };
            cpEntryValidators[innerClass.innerClassInfo()] =
                &IllegalStrippingTransformer::matchClass;
            // 0 if the defining class is the top-level class
            expectedTypeMasks[innerClass.outerClassInfo()] = [](int i) {
                return i == 0 || i == CP_CLASS;
            };
            // 0 if anonymous, otherwise name index
            expectedTypeMasks[innerClass.innerName()] = [](int i) {
                return i == 0 || i == CP_UTF8;
            };
        }
    }
    else if (name == AttributeConstants::CODE) {
        // Sanity check
        if (holderType != io::AttributeHolderType::METHOD) return false;
        // Method cannot be abstract or native (CAFED00D aa92cd61: native 方法
        // 不得携带 Code 属性)
        const auto* method = static_cast<const Method*>(&holder);
        if ((method->getAccess() & (ACC_ABSTRACT | ACC_NATIVE)) != 0) return false;

        auto* code = static_cast<CodeAttribute*>(const_cast<Attribute*>(attribute));

        // Prune bad code sub-attributes.
        auto& subAttrs = code->attributes();
        subAttrs.erase(
            std::remove_if(subAttrs.begin(),
                           subAttrs.end(),
                           [this, code](const Attribute* sub) { return !isValid(*code, sub); }),
            subAttrs.end());

        // Ensure exception indices are valid.
        for (const auto& entry : code->exceptionTable()) {
            const CpClass* catchType = entry.catchType();
            if (catchType == nullptr) {
                // A null catch_type is a catch-all handler and is implicitly valid.
                continue;
            }
            cpEntryValidators[catchType] = &IllegalStrippingTransformer::matchClass;
            expectedTypeMasks[catchType] = [](int i) { return i == CP_CLASS; };
        }
    }
    else if (name == AttributeConstants::SIGNATURE) {
        const auto*            attr          = static_cast<const SignatureAttribute*>(attribute);
        const AttributeHolder* holderPtr     = &holder;
        cpEntryValidators[attr->signature()] = [holderPtr](const CpEntry* e) {
            return matchSignature(static_cast<const CpUtf8*>(e), *holderPtr);
        };
    }
    else if (name == AttributeConstants::SOURCE_FILE) {
        const auto* attr = static_cast<const SourceFileAttribute*>(attribute);
        cpEntryValidators[attr->sourceFilename()] = &IllegalStrippingTransformer::matchUtf8NonEmpty;
    }
    else if (name == AttributeConstants::MODULE) {
        const auto* attr                   = static_cast<const ModuleAttribute*>(attribute);
        expectedTypeMasks[attr->version()] = [](int i) { return i == 0 || i == CP_UTF8; };
        for (const auto& req : attr->requires())
            expectedTypeMasks[req.version()] = [](int i) { return i == 0 || i == CP_UTF8; };
    }
    else if (name == AttributeConstants::BOOTSTRAP_METHODS) {
        auto*       attr    = static_cast<BootstrapMethodsAttribute*>(const_cast<Attribute*>(attribute));
        const auto& methods = attr->bootstrapMethods();
        std::vector<bool> retained(methods.size(), false);
        std::vector<BootstrapMethodsAttribute::BootstrapMethod> retainedMethods;
        retainedMethods.reserve(methods.size());

        // Filter out invalid bootstrap methods, and retain the valid ones.
        for (std::size_t i = 0; i < methods.size(); i++) {
            const auto&                             bsm   = methods[i];
            std::unordered_set<const ConstDynamic*> active;
            if (!isValidBootstrapMethod(&bsm, active)) continue;
            retained[i] = true;
            retainedMethods.push_back(bsm);
        }

        // Compact the table and rewrite every dynamic constant that used an original
        // index.
        if (retainedMethods.size() != methods.size()) {
            remapBootstrapMethodIndices(retained, pool_);
            attr->setBootstrapMethods(std::move(retainedMethods));
        }

        // Validate references from entries that survived filtering.
        for (const auto& bsm : attr->bootstrapMethods()) {
            expectedTypeMasks[bsm.bsmMethodRef()] = [](int i) { return i == CP_METHOD_HANDLE; };
            // Arguments must be loadable types.
            for (const CpEntry* arg : bsm.args())
                expectedTypeMasks[arg] = [](int i) {
                    return (i >= CP_INTEGER && i <= CP_STRING) ||
                           (i >= CP_METHOD_HANDLE && i <= CP_DYNAMIC);
                };
        }
    }
    else if (name == AttributeConstants::LOCAL_VARIABLE_TABLE) {
        const auto* attr = static_cast<const LocalVariableTableAttribute*>(attribute);
        for (const auto& entry : attr->entries()) {
            expectedTypeMasks[entry.name()] = [](int i) { return i == CP_UTF8; };
            expectedTypeMasks[entry.desc()] = [](int i) { return i == CP_UTF8; };
            cpEntryValidators[entry.name()] = [](const CpEntry* e) {
                return matchUtf8ValidUnqualifiedName(e) && matchUtf8Word(e);
            };
            cpEntryValidators[entry.desc()] =
                &IllegalStrippingTransformer::matchUtf8FieldDescriptor;
        }
    }
    else if (name == AttributeConstants::LOCAL_VARIABLE_TYPE_TABLE) {
        const auto* attr = static_cast<const LocalVariableTypeTableAttribute*>(attribute);
        for (const auto& entry : attr->entries()) {
            cpEntryValidators[entry.name()] = [](const CpEntry* e) {
                return matchUtf8ValidUnqualifiedName(e) && matchUtf8Word(e);
            };
            cpEntryValidators[entry.signature()] = &IllegalStrippingTransformer::matchUtf8NonEmpty;
        }
    }
    else if (name == AttributeConstants::PERMITTED_SUBCLASSES) {
        const auto* attr = static_cast<const PermittedClassesAttribute*>(attribute);
        for (const CpClass* index : attr->classes())
            cpEntryValidators[index] = &IllegalStrippingTransformer::matchClass;
    }
    else if (name == AttributeConstants::RECORD) {
        auto* attr = static_cast<RecordAttribute*>(const_cast<Attribute*>(attribute));
        for (auto& component :
             const_cast<std::vector<RecordAttribute::RecordComponent>&>(attr->components())) {
            cpEntryValidators[component.name()] = &IllegalStrippingTransformer::matchUtf8Word;
            cpEntryValidators[component.desc()] =
                &IllegalStrippingTransformer::matchUtf8FieldDescriptor;
            // Prune bad component sub-attributes (annotations).
            auto& subAttrs = component.attributes();
            subAttrs.erase(std::remove_if(subAttrs.begin(),
                                          subAttrs.end(),
                                          [this, &component](const Attribute* sub) {
                                              return !isValid(component, sub);
                                          }),
                           subAttrs.end());
        }
    }
    // Remaining attribute names (LineNumberTable, SourceDebugExtension, Deprecated,
    // Synthetic, CharacterRangeTable, CompilationID, MethodParameters, Module*,
    // SourceID, StackMapTable) carry no extra CP-reference checks here — matching
    // the Java reference, which leaves them as no-op / TODO.

    // Check referenced entries have the required constant-pool kind.
    for (const auto& kv : expectedTypeMasks) {
        const CpEntry* cpEntry = kv.first;

        // A null reference (index 0) is allowed iff the entry's own mask accepts 0,
        // which is used to indicate "not present" or other special cases in some
        // attributes. The masks that accept 0 are:
        //  - EnclosingMethod.method_index: 0 when the class is immediately enclosed (4.7.7)
        //  - InnerClasses.outer_class_info_index: 0 for top-level/local/anonymous (4.7.6)
        //  - InnerClasses.inner_name_index: 0 for anonymous classes (4.7.6)
        //  - Module.module_version_index: 0 when there is no version info (4.7.25)
        //  - Module.Requires.requires_version_index: 0 when there is no version info (4.7.25)
        if (cpEntry == nullptr) {
            if (kv.second(0)) continue;
            return false;
        }

        int tag = static_cast<int>(cpEntry->tag());
        if (!kv.second(tag)) return false;
    }

    // Apply content predicates independently of the type masks, so validators without
    // a mask still get applied（上游 a597006a）。
    for (const auto& kv : cpEntryValidators) {
        if (kv.first == nullptr) return false;
        if (!kv.second(kv.first)) return false;
    }
    return true;
}

void IllegalStrippingTransformer::deduplicateAttributes(AttributeHolder& holder)
{
    auto&                            attributes           = holder.attributes();
    std::unordered_map<std::string, AttrKind> singletonTypes;
    std::unordered_set<std::string>  conflictingSingletons;
    std::vector<const Attribute*>    normalized;
    normalized.reserve(attributes.size());
    for (const Attribute* attribute : attributes) {
        const std::string& attributeName = attribute->name().text();

        // Skip non-singleton attributes, they are allowed to be duplicated.
        if (!io::AttributeReader::isSingletonAttribute(attributeName)) {
            normalized.push_back(attribute);
            continue;
        }

        // Skip if we have already seen a conflicting singleton attribute, since they
        // are all discarded.
        if (conflictingSingletons.count(attributeName) > 0) continue;

        // Check if we have already seen a singleton attribute of this name, and if so
        // check that the type matches. C++ 无 RTTI：以 AttrKind 替代 Java 的
        // Class<?> 比较。
        auto  firstType = singletonTypes.find(attributeName);
        AttrKind kind   = attribute->kind();
        if (firstType == singletonTypes.end()) {
            singletonTypes.emplace(attributeName, kind);
            normalized.push_back(attribute);
        }
        else if (firstType->second == kind) {
            // Discarding duplicate singleton attribute.
        }
        else {
            // Discarding incompatible singleton attributes: drop every seen instance
            // of the name and mark the name as conflicting.
            normalized.erase(std::remove_if(normalized.begin(),
                                            normalized.end(),
                                            [&attributeName](const Attribute* existing) {
                                                return existing->name().text() == attributeName;
                                            }),
                             normalized.end());
            singletonTypes.erase(attributeName);
            conflictingSingletons.insert(attributeName);
        }
    }
    holder.setAttributes(std::move(normalized));

    // Recurse only into retained nested tables, which have independent cardinality
    // rules.
    for (const Attribute* attribute : holder.attributes()) {
        if (attribute->isCodeAttribute())
            deduplicateAttributes(
                const_cast<CodeAttribute&>(*static_cast<const CodeAttribute*>(attribute)));
        else if (attribute->name().text() == AttributeConstants::RECORD) {
            const auto* record = static_cast<const RecordAttribute*>(attribute);
            for (auto& component :
                 const_cast<std::vector<RecordAttribute::RecordComponent>&>(record->components()))
                deduplicateAttributes(component);
        }
    }
}

bool IllegalStrippingTransformer::isValidDynamic(const ConstDynamic*                     dynamic,
                                                 std::unordered_set<const ConstDynamic*>& active)
{
    if (dynamic == nullptr) return false;

    // A dynamic constant may syntactically reference itself (4.4.10), so guard against
    // cycles.
    if (!active.insert(dynamic).second) return true;

    // The name-and-type descriptor must match the constant kind.
    const CpUtf8* descriptor = dynamic->nameType() != nullptr ? dynamic->nameType()->type() : nullptr;
    bool validDescriptor = dynamic->tag() == CpTag::DYNAMIC
                               ? matchUtf8FieldDescriptor(descriptor)
                               : matchUtf8MethodDescriptor(descriptor);
    if (!validDescriptor) return false;

    // If there isn't a bootstrap method attribute, then there is no way to resolve the
    // dynamic constant.
    auto&          classAttrs = const_cast<std::vector<const Attribute*>&>(clazz_.attributes());
    const Attribute* attribute =
        findAttributeByName(classAttrs, AttributeConstants::BOOTSTRAP_METHODS);
    if (attribute == nullptr) return false;
    const auto* bsmAttribute = static_cast<const BootstrapMethodsAttribute*>(attribute);

    // Index must be in bounds, and the whole selected entry (including its arguments)
    // must be valid.
    int         index   = static_cast<int>(dynamic->bsmIndex());
    const auto& methods = bsmAttribute->bootstrapMethods();
    if (index < 0 || static_cast<std::size_t>(index) >= methods.size()) return false;
    return isValidBootstrapMethod(&methods[static_cast<std::size_t>(index)], active);
}

bool IllegalStrippingTransformer::isValidMethodHandle(const CpMethodHandle* handle) const
{
    if (handle == nullptr) return false;
    const ConstRef* reference = handle->reference();
    auto hasTag = [&](CpTag tag) { return reference != nullptr && reference->tag() == tag; };
    switch (handle->kind()) {
    case CpMethodHandle::REF_GET_FIELD:
    case CpMethodHandle::REF_GET_STATIC:
    case CpMethodHandle::REF_PUT_FIELD:
    case CpMethodHandle::REF_PUT_STATIC: return hasTag(CpTag::FIELD_REF);
    case CpMethodHandle::REF_INVOKE_VIRTUAL:
    case CpMethodHandle::REF_NEW_INVOKE_SPECIAL: return hasTag(CpTag::METHOD_REF);
    case CpMethodHandle::REF_INVOKE_STATIC:
    case CpMethodHandle::REF_INVOKE_SPECIAL:
        return hasTag(CpTag::METHOD_REF) ||
               (clazz_.getVersionMajor() >= 52 && hasTag(CpTag::INTERFACE_METHOD_REF));
    case CpMethodHandle::REF_INVOKE_INTERFACE: return hasTag(CpTag::INTERFACE_METHOD_REF);
    default: return false;
    }
}

bool IllegalStrippingTransformer::isValidLdcEntry(int opcode, const CpEntry* entry)
{
    // Check for wide + dynamic wide constants.
    if (opcode == Opcodes::LDC2_W) {
        return entry != nullptr &&
               (entry->tag() == CpTag::LONG || entry->tag() == CpTag::DOUBLE ||
                (entry->isConstDynamic() &&
                 isWideDynamic(static_cast<const ConstDynamic*>(entry))));
    }

    // Other LDC forms cannot load wide constants, and cannot load dynamic constants
    // with wide descriptors.
    if (entry == nullptr || !entry->isLoadableConstant() || entry->tag() == CpTag::LONG ||
        entry->tag() == CpTag::DOUBLE)
        return false;
    return !(entry->isConstDynamic() &&
             isWideDynamic(static_cast<const ConstDynamic*>(entry)));
}

bool IllegalStrippingTransformer::isWideDynamic(const ConstDynamic* dynamic)
{
    return dynamic->nameType() != nullptr && isWideType(dynamic->nameType()->type());
}

bool IllegalStrippingTransformer::isWideType(const CpUtf8* type)
{
    if (type == nullptr) return false;
    const auto& text = type->text();
    return text == "J" || text == "D";
}

bool IllegalStrippingTransformer::isValidTypeAnnotationTarget(
    const AttributeHolder* holder, const TypeAnnotation* typeAnnotation)
{
    // Nested annotation values are never type annotations, so a type annotation always
    // has a holder.
    if (holder == nullptr) return false;
    const TargetInfo* targetInfo = typeAnnotation->getTargetInfo();
    if (targetInfo == nullptr) return false;

    // The target_type must be permitted in the enclosing holder (JVMS 4.7.20-C).
    if (!io::isAnnotationTargetTypeValidIn(targetInfo->getTargetType(), holder->holderType()))
        return false;

    switch (targetInfo->getTargetTypeKind()) {
    case TargetInfoType::SUPERTYPE_TARGET:
    {
        if (holder->holderType() != io::AttributeHolderType::CLASS) return false;
        return isValidSuperTypeTarget(static_cast<const ClassFile*>(holder),
                                      static_cast<const SuperTypeTargetInfo*>(targetInfo));
    }
    case TargetInfoType::THROWS_TARGET:
    {
        if (holder->holderType() != io::AttributeHolderType::METHOD) return false;
        return isValidThrowsTarget(static_cast<const Method*>(holder),
                                   static_cast<const ThrowsTargetInfo*>(targetInfo));
    }
    case TargetInfoType::CATCH_TARGET:
    {
        if (holder->holderType() != io::AttributeHolderType::ATTRIBUTE) return false;
        return isValidCatchTarget(static_cast<const CodeAttribute*>(holder),
                                  static_cast<const CatchTargetInfo*>(targetInfo));
    }
    default:
        // TYPE_PARAMETER / TYPE_PARAMETER_BOUND / FORMAL_PARAMETER / LOCALVAR /
        // OFFSET / TYPE_ARGUMENT / EMPTY targets: no-op here（上游 TODO arms）。
        // LOCALVAR/OFFSET/TYPE_ARGUMENT 的区间规则在 removeInvalidTypeAnnotations
        // 中按 Code 上下文校验。
        return true;
    }
}

bool IllegalStrippingTransformer::isValidSuperTypeTarget(const ClassFile*           classFile,
                                                         const SuperTypeTargetInfo* info)
{
    // Special case for interfaces, which have no superclass, and for java/lang/Object,
    // which has no superclass.
    if (info->isExtends()) {
        return !Modifiers::has(static_cast<uint16_t>(classFile->getAccess()),
                               Modifiers::ACC_INTERFACE) &&
               classFile->getSuperClass() != nullptr;
    }

    int superTypeIndex = info->getSuperTypeIndex();
    return superTypeIndex >= 0 &&
           static_cast<std::size_t>(superTypeIndex) < classFile->getInterfaceClasses().size();
}

bool IllegalStrippingTransformer::isValidThrowsTarget(const Method*            method,
                                                      const ThrowsTargetInfo* info)
{
    const Attribute* attribute =
        findAttributeByName(method->attributes(), AttributeConstants::EXCEPTIONS);
    if (attribute == nullptr) return false;
    const auto* exceptions = static_cast<const ExceptionsAttribute*>(attribute);

    int throwsTypeIndex = info->getThrowsTypeIndex();
    return throwsTypeIndex >= 0 &&
           static_cast<std::size_t>(throwsTypeIndex) < exceptions->exceptionTable().size();
}

bool IllegalStrippingTransformer::isValidCatchTarget(const CodeAttribute*    code,
                                                     const CatchTargetInfo* info)
{
    int tableIndex = info->getExceptionTableIndex();
    return tableIndex >= 0 &&
           static_cast<std::size_t>(tableIndex) < code->exceptionTable().size();
}

void IllegalStrippingTransformer::addAnnotationValidation(const AttributeHolder* holder,
                                                          ExpectedTypeMasks&     expectedTypeMasks,
                                                          CpEntryValidators&     cpEntryValidators,
                                                          const Annotation*      anno)
{
    if (anno == nullptr) return;
    expectedTypeMasks[anno->typePtr()] = [](int i) { return i == CP_UTF8; };
    cpEntryValidators[anno->typePtr()] = &IllegalStrippingTransformer::matchUtf8FieldDescriptor;
    for (const auto& entry : anno->getValues()) {
        const CpUtf8* elementTypeIndex = entry.first;
        cpEntryValidators[elementTypeIndex] =
            &IllegalStrippingTransformer::matchUtf8ValidUnqualifiedName;
        addElementValueValidation(expectedTypeMasks, cpEntryValidators, entry.second.get());
    }

    if (anno->kind() == AnnotationKind::TypeAnnotation) {
        const auto* typeAnnotation = static_cast<const TypeAnnotation*>(anno);
        if (!isValidTypeAnnotationTarget(holder, typeAnnotation)) {
            // 无条件拒绝的 null-key 约定写法；mask 不接受 0 → 属性校验失败。
            expectedTypeMasks[nullptr] = [](int) { return false; };
        }
    }
}

void IllegalStrippingTransformer::addElementValueValidation(ExpectedTypeMasks&  expectedTypeMasks,
                                                            CpEntryValidators&  cpEntryValidators,
                                                            const ElementValue* elementValue)
{
    if (elementValue == nullptr) return;
    switch (elementValue->kind()) {
    case ElementValueKind::Class:
    {
        const auto* classElement = static_cast<const ClassElementValue*>(elementValue);
        // return descriptor：'V' 或字段类型，绝不是完整方法描述符
        // （上游 62af8af2）。
        cpEntryValidators[classElement->getClassEntry()] =
            &IllegalStrippingTransformer::matchUtf8ReturnDescriptor;
        break;
    }
    case ElementValueKind::Enum:
    {
        const auto* enumElement = static_cast<const EnumElementValue*>(elementValue);
        cpEntryValidators[enumElement->getType()] =
            &IllegalStrippingTransformer::matchUtf8FieldDescriptor;
        // 枚举常量名必须是非限定名（上游 f19d7129）。
        cpEntryValidators[enumElement->getName()] =
            &IllegalStrippingTransformer::matchUtf8ValidUnqualifiedName;
        break;
    }
    case ElementValueKind::Primitive:
    {
        const auto* primitive = static_cast<const PrimitiveElementValue*>(elementValue);
        // 常量池条目种类必须与元素 tag 声明的原始类型匹配（上游 66d31cf8）。
        int requiredTag = io::AnnotationReader::requiredConstantTagForPrimitive(
            primitive->getTag());
        expectedTypeMasks[primitive->getValue()] = [requiredTag](int i) {
            return i == requiredTag;
        };
        break;
    }
    case ElementValueKind::Annotation:
    {
        const auto* annotationElement = static_cast<const AnnotationElementValue*>(elementValue);
        addAnnotationValidation(
            nullptr, expectedTypeMasks, cpEntryValidators, annotationElement->getAnnotation());
        break;
    }
    case ElementValueKind::Array:
    {
        const auto* arrayElement = static_cast<const ArrayElementValue*>(elementValue);
        for (const auto& arrayValue : arrayElement->getArray())
            addElementValueValidation(expectedTypeMasks, cpEntryValidators, arrayValue.get());
        break;
    }
    case ElementValueKind::Utf8:
    default: break;
    }
}

bool IllegalStrippingTransformer::isBranch(const Instruction* insn)
{
    return CodeUtilities::isBranch(insn);
}

bool IllegalStrippingTransformer::isTerminalOrAlwaysTakeFlowControl(const Instruction* insn)
{
    return CodeUtilities::isTerminalOrAlwaysTakeFlowControl(insn);
}

bool IllegalStrippingTransformer::checkInvalidJumpUpTo(const CodeAttribute& code, int max)
{
    int offset = 0;
    for (const Instruction* insn : code.instructions()) {
        if (offset > max) break;
        if (isBranch(insn) && insn->kind() == InsnKind::IntOperand) {
            const auto* jump   = static_cast<const IntOperandInstruction*>(insn);
            int         target = offset + jump->getOperand();
            if (code.getInstructionAtOffset(target) == nullptr) return true;
        }
        offset += insn->computeSize();
    }
    return false;
}

bool IllegalStrippingTransformer::checkInvalidJumpFrom(const CodeAttribute& code, int min)
{
    int offset = 0;
    for (const Instruction* insn : code.instructions()) {
        if (offset > min && isBranch(insn) && insn->kind() == InsnKind::IntOperand) {
            const auto* jump   = static_cast<const IntOperandInstruction*>(insn);
            int         target = offset + jump->getOperand();
            if (code.getInstructionAtOffset(target) == nullptr) return true;
        }
        offset += insn->computeSize();
    }
    return false;
}

void IllegalStrippingTransformer::shiftJumps(std::vector<const Instruction*>& instructions,
                                             std::function<bool(int)> blockRelativeJumpPredicate,
                                             std::function<int(int, int)> rewriter)
{
    int insnOffset = 0;
    for (const Instruction* insn : instructions) {
        if (isBranch(insn) && insn->kind() == InsnKind::IntOperand) {
            auto* jump =
                const_cast<IntOperandInstruction*>(static_cast<const IntOperandInstruction*>(insn));
            if (blockRelativeJumpPredicate(insnOffset + jump->getOperand()))
                jump->setOperand(rewriter(insnOffset, jump->getOperand()));
        }
        else if (insn->kind() == InsnKind::LookupSwitch) {
            auto* switchInsn = const_cast<LookupSwitchInstruction*>(
                static_cast<const LookupSwitchInstruction*>(insn));
            int dflt = switchInsn->getDefault();
            if (blockRelativeJumpPredicate(insnOffset + dflt))
                switchInsn->setDefault(rewriter(insnOffset, dflt));
            auto offsets = const_cast<std::vector<int>&>(switchInsn->getOffsets());
            for (std::size_t i = 0; i < offsets.size(); i++) {
                if (blockRelativeJumpPredicate(insnOffset + offsets[i]))
                    offsets[i] = rewriter(insnOffset, offsets[i]);
            }
        }
        else if (insn->kind() == InsnKind::TableSwitch) {
            auto* switchInsn = const_cast<TableSwitchInstruction*>(
                static_cast<const TableSwitchInstruction*>(insn));
            int dflt = switchInsn->getDefault();
            if (blockRelativeJumpPredicate(insnOffset + dflt))
                switchInsn->setDefault(rewriter(insnOffset, dflt));
            auto offsets = const_cast<std::vector<int>&>(switchInsn->getOffsets());
            for (std::size_t i = 0; i < offsets.size(); i++) {
                if (blockRelativeJumpPredicate(insnOffset + offsets[i]))
                    offsets[i] = rewriter(insnOffset, offsets[i]);
            }
        }
        insnOffset += insn->computeSize();
    }
}

bool IllegalStrippingTransformer::isValidCodeRange(const CodeAttribute& code, int codeLength,
                                                   int startPc, int length)
{
    long long endPc = static_cast<long long>(startPc) + length;
    return startPc < codeLength && endPc <= codeLength &&
           isInstructionBoundary(code, codeLength, startPc) &&
           (endPc == codeLength || isInstructionBoundary(code, codeLength,
                                                         static_cast<int>(endPc)));
}

bool IllegalStrippingTransformer::isInstructionBoundary(const CodeAttribute& code, int codeLength,
                                                        int offset)
{
    return offset >= 0 && offset < codeLength && code.getInstructionAtOffset(offset) != nullptr;
}

int IllegalStrippingTransformer::computeStack(const Instruction* insn)
{
    using namespace Opcodes;
    switch (insn->getOpcode()) {
    case ACONST_NULL:
    case ICONST_M1:
    case ICONST_0:
    case ICONST_1:
    case ICONST_2:
    case ICONST_3:
    case ICONST_4:
    case ICONST_5:
    case FCONST_0:
    case FCONST_1:
    case FCONST_2:
    case BIPUSH:
    case SIPUSH:
    case ILOAD:
    case ILOAD_0:
    case ILOAD_1:
    case ILOAD_2:
    case ILOAD_3:
    case FLOAD:
    case FLOAD_0:
    case FLOAD_1:
    case FLOAD_2:
    case FLOAD_3:
    case LDC: return 1;
    case LCONST_0:
    case LCONST_1:
    case DCONST_0:
    case DCONST_1:
    case LLOAD:
    case LLOAD_0:
    case LLOAD_1:
    case LLOAD_2:
    case LLOAD_3:
    case DLOAD:
    case DLOAD_0:
    case DLOAD_1:
    case DLOAD_2:
    case DLOAD_3:
    case LDC2_W: return 2;
    case IALOAD:
    case LALOAD:
    case FALOAD:
    case DALOAD:
    case AALOAD:
    case BALOAD:
    case CALOAD:
    case SALOAD: return -1;
    case ISTORE:
    case ISTORE_0:
    case ISTORE_1:
    case ISTORE_2:
    case ISTORE_3:
    case FSTORE:
    case FSTORE_0:
    case FSTORE_1:
    case FSTORE_2:
    case FSTORE_3:
    case ASTORE:
    case ASTORE_0:
    case ASTORE_1:
    case ASTORE_2:
    case ASTORE_3:
    case POP:
    case PUTSTATIC: return -1;
    case LSTORE:
    case LSTORE_0:
    case LSTORE_1:
    case LSTORE_2:
    case LSTORE_3:
    case DSTORE:
    case DSTORE_0:
    case DSTORE_1:
    case DSTORE_2:
    case DSTORE_3:
    case POP2: return -2;
    case DUP: return 1;
    case DUP_X1: return 1;
    case DUP_X2: return 1;
    case DUP2: return 2;
    case DUP2_X1: return 2;
    case DUP2_X2: return 2;
    default: return 0;
    }
}

int IllegalStrippingTransformer::computeLocals(const Instruction* insn)
{
    using namespace Opcodes;
    switch (insn->getOpcode()) {
    case ILOAD:
    case FLOAD:
    case ALOAD: return 1;
    case LLOAD:
    case DLOAD: return 2;
    case ISTORE:
    case FSTORE:
    case ASTORE: return -1;
    case LSTORE:
    case DSTORE: return -2;
    case IINC: return 1;
    case ILOAD_0:
    case ILOAD_1:
    case ILOAD_2:
    case ILOAD_3:
    case FLOAD_0:
    case FLOAD_1:
    case FLOAD_2:
    case FLOAD_3:
    case ALOAD_0:
    case ALOAD_1:
    case ALOAD_2:
    case ALOAD_3: return 1;
    case LLOAD_0:
    case LLOAD_1:
    case LLOAD_2:
    case LLOAD_3:
    case DLOAD_0:
    case DLOAD_1:
    case DLOAD_2:
    case DLOAD_3: return 2;
    case ISTORE_0:
    case ISTORE_1:
    case ISTORE_2:
    case ISTORE_3:
    case FSTORE_0:
    case FSTORE_1:
    case FSTORE_2:
    case FSTORE_3:
    case ASTORE_0:
    case ASTORE_1:
    case ASTORE_2:
    case ASTORE_3: return -1;
    case LSTORE_0:
    case LSTORE_1:
    case LSTORE_2:
    case LSTORE_3:
    case DSTORE_0:
    case DSTORE_1:
    case DSTORE_2:
    case DSTORE_3: return -2;
    default: return 0;
    }
}

bool IllegalStrippingTransformer::isValidUnqualifiedName(std::string_view text)
{
    // Cannot be empty.
    if (text.empty()) return false;

    // From #jvms-4.2.2 - An unqualified name must not contain '.', ';', '[', or '/'.
    for (char c : text) {
        if (c == '.' || c == ';' || c == '[' || c == '/') return false;
    }
    return true;
}

bool IllegalStrippingTransformer::isValidClassName(std::string_view text)
{
    if (text.empty()) return false;

    // A binary name in internal form splits into non-empty unqualified-name components
    // on '/'.
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = text.find('/', start);
        if (end == std::string_view::npos) end = text.size();
        if (!isValidUnqualifiedName(text.substr(start, end - start))) return false;
        start = end + 1;
    }
    return true;
}

int IllegalStrippingTransformer::constantValueTag(const std::string& fieldDesc)
{
    // JVMS 4.7.2, Table 4.7.2-A.
    if (fieldDesc == "Z" || fieldDesc == "B" || fieldDesc == "C" || fieldDesc == "S" ||
        fieldDesc == "I")
        return CP_INTEGER;
    if (fieldDesc == "F") return CP_FLOAT;
    if (fieldDesc == "J") return CP_LONG;
    if (fieldDesc == "D") return CP_DOUBLE;
    if (fieldDesc == "Ljava/lang/String;") return CP_STRING;
    return -1;
}

bool IllegalStrippingTransformer::matchClass(const CpEntry* e)
{
    if (!e || e->tag() != CpTag::CLASS) return false;
    const auto* cpClass = static_cast<const CpClass*>(e);
    return matchUtf8ValidClassName(cpClass->name());
}

bool IllegalStrippingTransformer::matchUtf8ValidClassName(const CpEntry* e)
{
    if (!e || e->tag() != CpTag::UTF8) return false;
    return isValidClassName(static_cast<const CpUtf8*>(e)->text());
}

bool IllegalStrippingTransformer::matchUtf8ValidUnqualifiedName(const CpEntry* e)
{
    if (!e || e->tag() != CpTag::UTF8) return false;
    return isValidUnqualifiedName(static_cast<const CpUtf8*>(e)->text());
}

bool IllegalStrippingTransformer::matchUtf8FieldDescriptor(const CpEntry* e)
{
    if (!e || e->tag() != CpTag::UTF8) return false;
    Descriptor d = Descriptor::from(static_cast<const CpUtf8*>(e)->text());
    switch (d.getKind()) {
    case Descriptor::Kind::Object:
    case Descriptor::Kind::Array: return true;
    case Descriptor::Kind::Primitive: return d.getDescriptor() != "V";   // Void doesn't count.
    default: return false;
    }
}

bool IllegalStrippingTransformer::matchUtf8MethodDescriptor(const CpEntry* e)
{
    if (!e || e->tag() != CpTag::UTF8) return false;
    return Descriptor::from(static_cast<const CpUtf8*>(e)->text()).getKind() ==
           Descriptor::Kind::Method;
}

bool IllegalStrippingTransformer::matchUtf8ReturnDescriptor(const CpEntry* e)
{
    if (!e || e->tag() != CpTag::UTF8) return false;

    // A return descriptor is 'V' or a field type, never a full method descriptor.
    Descriptor d = Descriptor::from(static_cast<const CpUtf8*>(e)->text());
    switch (d.getKind()) {
    case Descriptor::Kind::Primitive:
    case Descriptor::Kind::Object:
    case Descriptor::Kind::Array: return true;
    default: return false;
    }
}

bool IllegalStrippingTransformer::matchUtf8NonEmpty(const CpEntry* e)
{
    if (!e || e->tag() != CpTag::UTF8) return false;
    return !static_cast<const CpUtf8*>(e)->text().empty();
}

bool IllegalStrippingTransformer::matchUtf8Word(const CpEntry* e)
{
    if (!e || e->tag() != CpTag::UTF8) return false;
    const auto& text = static_cast<const CpUtf8*>(e)->text();
    if (text.empty()) return false;
    for (char c : text) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '<' || c == '>' || c == ';' ||
              c == '/' || c == '$' || c == '_'))
            return false;
    }
    return true;
}

bool IllegalStrippingTransformer::matchSignature(const CpUtf8* e, const AttributeHolder& context)
{
    (void)context;
    return matchUtf8NonEmpty(e);
}

}   // namespace jclassfile::transform
