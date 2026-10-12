// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/io/AttributeWriter.java
// 状态：阶段 5c + 阶段 6 完整实现
// 备注：属性名和 Attribute 具体类型由模型构造器共同保证；分派后使用 static_cast，
// 保持项目无 RTTI 约束。Code 与注解属性分别委派给专用写入器。

#include "jclassfile/io/attribute_writer.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "jclassfile/io/annotation_writer.hpp"
#include "jclassfile/io/instruction_writer.hpp"
#include "libca/core/datatype.hpp"
#include "jclassfile/model/attribute/annotation_default_attribute.hpp"
#include "jclassfile/model/attribute/annotations_attribute.hpp"
#include "jclassfile/model/attribute/attribute_constants.hpp"
#include "jclassfile/model/attribute/bootstrap_methods_attribute.hpp"
#include "jclassfile/model/attribute/character_range_table_attribute.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/compilation_id_attribute.hpp"
#include "jclassfile/model/attribute/constant_value_attribute.hpp"
#include "jclassfile/model/attribute/default_attribute.hpp"
#include "jclassfile/model/attribute/enclosing_method_attribute.hpp"
#include "jclassfile/model/attribute/exceptions_attribute.hpp"
#include "jclassfile/model/attribute/inner_classes_attribute.hpp"
#include "jclassfile/model/attribute/line_number_table_attribute.hpp"
#include "jclassfile/model/attribute/local_variable_table_attribute.hpp"
#include "jclassfile/model/attribute/local_variable_type_table_attribute.hpp"
#include "jclassfile/model/attribute/method_parameters_attribute.hpp"
#include "jclassfile/model/attribute/module_attribute.hpp"
#include "jclassfile/model/attribute/module_hashes_attribute.hpp"
#include "jclassfile/model/attribute/module_main_class_attribute.hpp"
#include "jclassfile/model/attribute/module_packages_attribute.hpp"
#include "jclassfile/model/attribute/module_resolution_attribute.hpp"
#include "jclassfile/model/attribute/module_target_attribute.hpp"
#include "jclassfile/model/attribute/nest_host_attribute.hpp"
#include "jclassfile/model/attribute/nest_members_attribute.hpp"
#include "jclassfile/model/attribute/parameter_annotations_attribute.hpp"
#include "jclassfile/model/attribute/permitted_classes_attribute.hpp"
#include "jclassfile/model/attribute/record_attribute.hpp"
#include "jclassfile/model/attribute/signature_attribute.hpp"
#include "jclassfile/model/attribute/source_debug_extension_attribute.hpp"
#include "jclassfile/model/attribute/source_file_attribute.hpp"
#include "jclassfile/model/attribute/source_id_attribute.hpp"
#include "jclassfile/model/attribute/stack_map_table_attribute.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_module.hpp"
#include "jclassfile/model/constant/cp_package.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/util/growing_byte_buffer.hpp"

namespace jclassfile::io {

using namespace jclassfile::model;

int AttributeWriter::orZero(const CpEntry* entry) const noexcept
{
    if (entry == nullptr) return 0;
    return entry->index();
}

std::vector<ca::u8> AttributeWriter::writeAttribute(const Attribute& attribute)
{
    util::GrowingByteBuffer baos;
    baos.ensureWriteable(6);
    if (attribute.isDefaultAttribute()) {
        const auto* dflt = static_cast<const DefaultAttribute*>(&attribute);
        baos.putShort(dflt->name().index());
        const auto& data = dflt->data();
        baos.putInt(static_cast<int>(data.size()));
        if (!data.empty()) {
            for (ca::u8 b : data) baos.put(b);
        }
        baos.trim();
        return baos.unwrap();
    }

    const CpUtf8& cpName = attribute.name();
    baos.putShort(cpName.index());
    baos.putInt(attribute.computeInternalLength());

    const std::string attrName = cpName.text();
    using namespace std::string_view_literals;

    // Attribute::name() 是模型层的类型契约。每个分支只处理对应派生类型，
    // 因此无需 dynamic_cast；契约被破坏属于调用方构造模型错误。
    if (attrName == AttributeConstants::BOOTSTRAP_METHODS) {
        const auto& bsms = static_cast<const BootstrapMethodsAttribute&>(attribute);
        baos.putShort(static_cast<int>(bsms.bootstrapMethods().size()));
        for (const auto& bsm : bsms.bootstrapMethods()) {
            const CpEntry* methodRef = bsm.bsmMethodRef();
            if (methodRef == nullptr) {
                throw std::invalid_argument("BootstrapMethod has null methodRef");
            }
            baos.putShort(methodRef->index());
            baos.putShort(static_cast<int>(bsm.args().size()));
            for (const CpEntry* arg : bsm.args()) {
                baos.putShort(arg == nullptr ? 0 : arg->index());
            }
        }
    }
    else if (attrName == AttributeConstants::CODE) {
        const auto& code = static_cast<const CodeAttribute&>(attribute);
        baos.putShort(code.maxStack());
        baos.putShort(code.maxLocals());
        InstructionWriter   insnWriter(fallback_);
        std::vector<ca::u8> codeBytes = insnWriter.writeCode(code.instructions());
        baos.putInt(static_cast<int>(codeBytes.size()));
        for (ca::u8 b : codeBytes) baos.put(b);
        baos.putShort(static_cast<int>(code.exceptionTable().size()));
        for (const auto& tableEntry : code.exceptionTable()) {
            baos.putShort(tableEntry.startPc());
            baos.putShort(tableEntry.endPc());
            baos.putShort(tableEntry.handlerPc());
            baos.putShort(orZero(tableEntry.catchType()));
        }
        baos.putShort(static_cast<int>(code.attributes().size()));
        for (const Attribute* sub : code.attributes()) {
            std::vector<ca::u8> subBytes = writeAttribute(*sub);
            for (ca::u8 b : subBytes) baos.put(b);
        }
    }
    else if (attrName == AttributeConstants::CONSTANT_VALUE) {
        const auto& cva = static_cast<const ConstantValueAttribute&>(attribute);
        baos.putShort(orZero(cva.constantValue()));
    }
    else if (attrName == AttributeConstants::ENCLOSING_METHOD) {
        const auto& ema = static_cast<const EnclosingMethodAttribute&>(attribute);
        baos.putShort(ema.classEntry() == nullptr ? 0 : ema.classEntry()->index());
        baos.putShort(orZero(ema.methodEntry()));
    }
    else if (attrName == AttributeConstants::EXCEPTIONS) {
        const auto& ea = static_cast<const ExceptionsAttribute&>(attribute);
        baos.putShort(static_cast<int>(ea.exceptionTable().size()));
        for (const CpClass* c : ea.exceptionTable()) {
            baos.putShort(c == nullptr ? 0 : c->index());
        }
    }
    else if (attrName == AttributeConstants::INNER_CLASSES) {
        const auto& ica = static_cast<const InnerClassesAttribute&>(attribute);
        baos.putShort(static_cast<int>(ica.innerClasses().size()));
        for (const auto& ic : ica.innerClasses()) {
            baos.putShort(ic.innerClassInfo() == nullptr ? 0 : ic.innerClassInfo()->index());
            baos.putShort(orZero(ic.outerClassInfo()));
            baos.putShort(orZero(ic.innerName()));
            baos.putShort(ic.innerClassAccessFlags());
        }
    }
    else if (attrName == AttributeConstants::LINE_NUMBER_TABLE) {
        const auto& lnt = static_cast<const LineNumberTableAttribute&>(attribute);
        baos.putShort(static_cast<int>(lnt.entries().size()));
        for (const auto& entry : lnt.entries()) {
            baos.putShort(entry.startPc());
            baos.putShort(entry.line());
        }
    }
    else if (attrName == AttributeConstants::LOCAL_VARIABLE_TABLE) {
        const auto& lvt = static_cast<const LocalVariableTableAttribute&>(attribute);
        baos.putShort(static_cast<int>(lvt.entries().size()));
        for (const auto& entry : lvt.entries()) {
            baos.putShort(entry.startPc());
            baos.putShort(entry.length());
            baos.putShort(entry.name() == nullptr ? 0 : entry.name()->index());
            baos.putShort(entry.desc() == nullptr ? 0 : entry.desc()->index());
            baos.putShort(entry.index());
        }
    }
    else if (attrName == AttributeConstants::LOCAL_VARIABLE_TYPE_TABLE) {
        const auto& lvtt = static_cast<const LocalVariableTypeTableAttribute&>(attribute);
        baos.putShort(static_cast<int>(lvtt.entries().size()));
        for (const auto& entry : lvtt.entries()) {
            baos.putShort(entry.startPc());
            baos.putShort(entry.length());
            baos.putShort(entry.name() == nullptr ? 0 : entry.name()->index());
            baos.putShort(entry.signature() == nullptr ? 0 : entry.signature()->index());
            baos.putShort(entry.index());
        }
    }
    else if (attrName == AttributeConstants::METHOD_PARAMETERS) {
        const auto& mpa = static_cast<const MethodParametersAttribute&>(attribute);
        baos.put(static_cast<int>(mpa.parameters().size()));
        for (const auto& parameter : mpa.parameters()) {
            baos.putShort(orZero(parameter.name()));
            baos.putShort(parameter.accessFlags());
        }
    }
    else if (attrName == AttributeConstants::MODULE) {
        const auto&    mod      = static_cast<const ModuleAttribute&>(attribute);
        const CpEntry* modEntry = mod.module();
        baos.putShort(modEntry == nullptr ? 0 : modEntry->index());
        baos.putShort(mod.flags());
        baos.putShort(orZero(mod.version()));
        baos.putShort(static_cast<int>(mod.requires().size()));
        for (const auto& r : mod.requires()) {
            baos.putShort(r.module() == nullptr ? 0 : r.module()->index());
            baos.putShort(r.flags());
            baos.putShort(orZero(r.version()));
        }
        baos.putShort(static_cast<int>(mod.exports().size()));
        for (const auto& e : mod.exports()) {
            baos.putShort(e.packageEntry() == nullptr ? 0 : e.packageEntry()->index());
            baos.putShort(e.flags());
            baos.putShort(static_cast<int>(e.to().size()));
            for (const CpModule* m : e.to()) {
                baos.putShort(m == nullptr ? 0 : m->index());
            }
        }
        baos.putShort(static_cast<int>(mod.opens().size()));
        for (const auto& o : mod.opens()) {
            baos.putShort(o.packageEntry() == nullptr ? 0 : o.packageEntry()->index());
            baos.putShort(o.flags());
            baos.putShort(static_cast<int>(o.to().size()));
            for (const CpModule* m : o.to()) {
                baos.putShort(m == nullptr ? 0 : m->index());
            }
        }
        baos.putShort(static_cast<int>(mod.uses().size()));
        for (const CpClass* u : mod.uses()) {
            baos.putShort(u == nullptr ? 0 : u->index());
        }
        baos.putShort(static_cast<int>(mod.provides().size()));
        for (const auto& p : mod.provides()) {
            baos.putShort(p.module() == nullptr ? 0 : p.module()->index());
            baos.putShort(static_cast<int>(p.with().size()));
            for (const CpClass* w : p.with()) {
                baos.putShort(w == nullptr ? 0 : w->index());
            }
        }
    }
    else if (attrName == AttributeConstants::NEST_HOST) {
        const auto& nh = static_cast<const NestHostAttribute&>(attribute);
        baos.putShort(nh.hostClass() == nullptr ? 0 : nh.hostClass()->index());
    }
    else if (attrName == AttributeConstants::NEST_MEMBERS) {
        const auto& nm = static_cast<const NestMembersAttribute&>(attribute);
        baos.putShort(static_cast<int>(nm.memberClasses().size()));
        for (const CpClass* c : nm.memberClasses()) {
            baos.putShort(c == nullptr ? 0 : c->index());
        }
    }
    else if (attrName == AttributeConstants::RECORD) {
        const auto& rec = static_cast<const RecordAttribute&>(attribute);
        baos.putShort(static_cast<int>(rec.components().size()));
        for (const auto& c : rec.components()) {
            baos.putShort(c.name() == nullptr ? 0 : c.name()->index());
            baos.putShort(c.desc() == nullptr ? 0 : c.desc()->index());
            baos.putShort(static_cast<int>(c.attributes().size()));
            for (const Attribute* sub : c.attributes()) {
                std::vector<ca::u8> subBytes = writeAttribute(*sub);
                for (ca::u8 b : subBytes) baos.put(b);
            }
        }
    }
    else if (attrName == AttributeConstants::RUNTIME_VISIBLE_ANNOTATIONS ||
             attrName == AttributeConstants::RUNTIME_INVISIBLE_ANNOTATIONS) {
        AnnotationWriter aw(baos);
        aw.writeAnnotations(static_cast<const AnnotationsAttribute&>(attribute));
    }
    else if (attrName == AttributeConstants::RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS ||
             attrName == AttributeConstants::RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS) {
        AnnotationWriter aw(baos);
        aw.writeParameterAnnotations(static_cast<const ParameterAnnotationsAttribute&>(attribute));
    }
    else if (attrName == AttributeConstants::RUNTIME_VISIBLE_TYPE_ANNOTATIONS ||
             attrName == AttributeConstants::RUNTIME_INVISIBLE_TYPE_ANNOTATIONS) {
        AnnotationWriter aw(baos);
        aw.writeTypeAnnotations(static_cast<const AnnotationsAttribute&>(attribute));
    }
    else if (attrName == AttributeConstants::ANNOTATION_DEFAULT) {
        AnnotationWriter aw(baos);
        aw.writeAnnotationDefault(static_cast<const AnnotationDefaultAttribute&>(attribute));
    }
    else if (attrName == AttributeConstants::PERMITTED_SUBCLASSES) {
        const auto& pc = static_cast<const PermittedClassesAttribute&>(attribute);
        baos.putShort(static_cast<int>(pc.classes().size()));
        for (const CpClass* c : pc.classes()) {
            baos.putShort(c == nullptr ? 0 : c->index());
        }
    }
    else if (attrName == AttributeConstants::SIGNATURE) {
        const auto& sa = static_cast<const SignatureAttribute&>(attribute);
        baos.putShort(sa.signature() == nullptr ? 0 : sa.signature()->index());
    }
    else if (attrName == AttributeConstants::SOURCE_DEBUG_EXTENSION) {
        const auto& sde  = static_cast<const SourceDebugExtensionAttribute&>(attribute);
        const auto& data = sde.debugExtension();
        for (ca::u8 b : data) baos.put(b);
    }
    else if (attrName == AttributeConstants::SOURCE_FILE) {
        const auto& sfa = static_cast<const SourceFileAttribute&>(attribute);
        baos.putShort(sfa.sourceFilename() == nullptr ? 0 : sfa.sourceFilename()->index());
    }
    else if (attrName == AttributeConstants::STACK_MAP_TABLE) {
        const auto& smt = static_cast<const StackMapTableAttribute&>(attribute);
        baos.putShort(static_cast<int>(smt.frames().size()));
        for (const auto& frame : smt.frames()) {
            baos.put(frame->getFrameType());
            int ft = frame->getFrameType();
            if (ft >= StackMapTableConstants::SAME_FRAME_MIN &&
                ft <= StackMapTableConstants::SAME_FRAME_MAX) {}
            else if (ft >= StackMapTableConstants::SAME_LOCALS_ONE_STACK_ITEM_MIN &&
                     ft <= StackMapTableConstants::SAME_LOCALS_ONE_STACK_ITEM_MAX) {
                const auto& s =
                    static_cast<const StackMapTableAttribute::SameLocalsOneStackItem&>(*frame);
                writeVerificationType(baos, *s.stack());
            }
            else if (ft == StackMapTableConstants::SAME_LOCALS_ONE_STACK_ITEM_EXTENDED_MIN) {
                const auto& s =
                    static_cast<const StackMapTableAttribute::SameLocalsOneStackItemExtended&>(
                        *frame);
                baos.putShort(s.offsetDelta());
                writeVerificationType(baos, *s.stack());
            }
            else if (ft >= StackMapTableConstants::CHOP_FRAME_MIN &&
                     ft <= StackMapTableConstants::CHOP_FRAME_MAX) {
                const auto& c = static_cast<const StackMapTableAttribute::ChopFrame&>(*frame);
                baos.putShort(c.offsetDelta());
            }
            else if (ft == StackMapTableConstants::SAME_FRAME_EXTENDED_MIN) {
                const auto& s =
                    static_cast<const StackMapTableAttribute::SameFrameExtended&>(*frame);
                baos.putShort(s.offsetDelta());
            }
            else if (ft >= StackMapTableConstants::APPEND_FRAME_MIN &&
                     ft <= StackMapTableConstants::APPEND_FRAME_MAX) {
                const auto& a = static_cast<const StackMapTableAttribute::AppendFrame&>(*frame);
                baos.putShort(a.offsetDelta());
                for (const auto& t : a.additionalLocals()) {
                    writeVerificationType(baos, *t);
                }
            }
            else if (ft == StackMapTableConstants::FULL_FRAME_MIN) {
                const auto& f = static_cast<const StackMapTableAttribute::FullFrame&>(*frame);
                baos.putShort(f.offsetDelta());
                baos.putShort(static_cast<int>(f.locals().size()));
                for (const auto& t : f.locals()) {
                    writeVerificationType(baos, *t);
                }
                baos.putShort(static_cast<int>(f.stack().size()));
                for (const auto& t : f.stack()) {
                    writeVerificationType(baos, *t);
                }
            }
        }
    }
    else if (attrName == AttributeConstants::MODULE_PACKAGES) {
        const auto& mp = static_cast<const ModulePackagesAttribute&>(attribute);
        baos.putShort(static_cast<int>(mp.packages().size()));
        for (const CpPackage* p : mp.packages()) {
            baos.putShort(p == nullptr ? 0 : p->index());
        }
    }
    else if (attrName == AttributeConstants::MODULE_TARGET) {
        const auto& mt = static_cast<const ModuleTargetAttribute&>(attribute);
        baos.putShort(mt.platformName() == nullptr ? 0 : mt.platformName()->index());
    }
    else if (attrName == AttributeConstants::MODULE_HASHES) {
        const auto& mh = static_cast<const ModuleHashesAttribute&>(attribute);
        baos.putShort(mh.algorithmName() == nullptr ? 0 : mh.algorithmName()->index());
        baos.putShort(static_cast<int>(mh.moduleHashes().size()));
        for (const auto& [name, hash] : mh.moduleHashes()) {
            baos.putShort(name == nullptr ? 0 : name->index());
            baos.putShort(static_cast<int>(hash.size()));
            for (ca::u8 b : hash) baos.put(b);
        }
    }
    else if (attrName == AttributeConstants::MODULE_MAIN_CLASS) {
        const auto& mmc = static_cast<const ModuleMainClassAttribute&>(attribute);
        baos.putShort(mmc.mainClass() == nullptr ? 0 : mmc.mainClass()->index());
    }
    else if (attrName == AttributeConstants::MODULE_RESOLUTION) {
        const auto& mr = static_cast<const ModuleResolutionAttribute&>(attribute);
        baos.putShort(mr.flags());
    }
    else if (attrName == AttributeConstants::COMPILATION_ID) {
        const auto& cid = static_cast<const CompilationIdAttribute&>(attribute);
        baos.putShort(cid.compilationId() == nullptr ? 0 : cid.compilationId()->index());
    }
    else if (attrName == AttributeConstants::SOURCE_ID) {
        const auto& sid = static_cast<const SourceIdAttribute&>(attribute);
        baos.putShort(sid.sourceId() == nullptr ? 0 : sid.sourceId()->index());
    }
    else if (attrName == AttributeConstants::CHARACTER_RANGE_TABLE) {
        const auto& crt   = static_cast<const CharacterRangeTableAttribute&>(attribute);
        const auto& table = crt.characterRangeTable();
        baos.putShort(static_cast<int>(table.size()));
        for (const auto& info : table) {
            baos.putShort(info.startPc());
            baos.putShort(info.endPc());
            baos.putInt(info.characterRangeStart());
            baos.putInt(info.characterRangeEnd());
            baos.putShort(info.flags());
        }
    }
    else if (attrName == AttributeConstants::DEPRECATED ||
             attrName == AttributeConstants::SYNTHETIC) {
        // empty body
    }

    baos.trim();
    return baos.unwrap();
}

void writeVerificationType(util::GrowingByteBuffer&                out,
                           const StackMapTableAttribute::TypeInfo& type)
{
    out.put(type.getTag());
    if (type.getTag() == StackMapTableConstants::ITEM_OBJECT) {
        const auto& objVar = static_cast<const StackMapTableAttribute::ObjectVariableInfo&>(type);
        out.putShort(objVar.classEntry() == nullptr ? 0 : objVar.classEntry()->index());
    }
    else if (type.getTag() == StackMapTableConstants::ITEM_UNINITIALIZED) {
        const auto& uninitVar =
            static_cast<const StackMapTableAttribute::UninitializedVariableInfo&>(type);
        out.putShort(uninitVar.offset());
    }
}

}   // namespace jclassfile::io
