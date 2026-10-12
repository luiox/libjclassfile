/// @file        jclassfile.hpp
/// @author      canrad (1517807724@qq.com)
/// @brief       libjclassfile 统一入口头文件
/// @version     0.1
/// @date        2026-06-11
/// @copyright   Copyright (c) 2026
///
/// 包含 libjclassfile 全部公共 API：
///   - model/       ClassFile 数据模型（类、字段、方法、常量池、指令、属性、注解等）
///   - io/          读写器（ClassFileReader/Writer、AttributeReader/Writer 等）
///   - transform/   变换器（非法指令剥离、重写等）
///   - util/        工具（opcode 工具、字节缓冲区等）
///
/// 可按需单独包含子模块头文件以减少编译时间。
///
/// 使用方式（库代码以外引用此库）：
///   #include "jclassfile/jclassfile.hpp"       // 包含全部 API
#pragma once

// ============================================================================
// model/ — ClassFile 数据模型
// ============================================================================

// model/ 顶层
#include "jclassfile/model/class_file.hpp"
#include "jclassfile/model/class_member.hpp"
#include "jclassfile/model/const_pool.hpp"
#include "jclassfile/model/descriptor.hpp"
#include "jclassfile/model/field.hpp"
#include "jclassfile/model/method.hpp"
#include "jclassfile/model/modifiers.hpp"
#include "jclassfile/model/accessible.hpp"
#include "jclassfile/model/constant_pool_constants.hpp"
#include "jclassfile/model/invalid_cp_index_exception.hpp"

// model/annotation/ — 注解模型
#include "jclassfile/model/annotation/annotation.hpp"
#include "jclassfile/model/annotation/annotation_constants.hpp"
#include "jclassfile/model/annotation/annotation_element_value.hpp"
#include "jclassfile/model/annotation/array_element_value.hpp"
#include "jclassfile/model/annotation/class_element_value.hpp"
#include "jclassfile/model/annotation/element_value.hpp"
#include "jclassfile/model/annotation/element_value_constants.hpp"
#include "jclassfile/model/annotation/enum_element_value.hpp"
#include "jclassfile/model/annotation/primitive_element_value.hpp"
#include "jclassfile/model/annotation/target_info.hpp"
#include "jclassfile/model/annotation/target_info_type.hpp"
#include "jclassfile/model/annotation/type_annotation.hpp"
#include "jclassfile/model/annotation/type_path.hpp"
#include "jclassfile/model/annotation/type_path_element.hpp"
#include "jclassfile/model/annotation/type_path_kind.hpp"
#include "jclassfile/model/annotation/utf8_element_value.hpp"

// model/attribute/ — 属性模型
#include "jclassfile/model/attribute/attribute.hpp"
#include "jclassfile/model/attribute/attribute_constants.hpp"
#include "jclassfile/model/attribute/attribute_contexts.hpp"
#include "jclassfile/model/attribute/attribute_versions.hpp"
#include "jclassfile/model/attribute/code_attribute.hpp"
#include "jclassfile/model/attribute/code_utilities.hpp"
#include "jclassfile/model/attribute/version_constants.hpp"
#include "jclassfile/model/attribute/stack_map_table_constants.hpp"
#include "jclassfile/model/attribute/stack_map_table_attribute.hpp"
#include "jclassfile/model/attribute/annotation_default_attribute.hpp"
#include "jclassfile/model/attribute/annotations_attribute.hpp"
#include "jclassfile/model/attribute/bootstrap_methods_attribute.hpp"
#include "jclassfile/model/attribute/character_range_table_attribute.hpp"
#include "jclassfile/model/attribute/compilation_id_attribute.hpp"
#include "jclassfile/model/attribute/constant_value_attribute.hpp"
#include "jclassfile/model/attribute/default_attribute.hpp"
#include "jclassfile/model/attribute/deprecated_attribute.hpp"
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
#include "jclassfile/model/attribute/synthetic_attribute.hpp"

// model/behavior/ — 行为接口
#include "jclassfile/model/behavior/attribute_holder.hpp"
#include "jclassfile/model/behavior/cp_accessor.hpp"

// model/constant/ — 常量池条目
#include "jclassfile/model/constant/const_ref.hpp"
#include "jclassfile/model/constant/const_ref_internal.hpp"
#include "jclassfile/model/constant/const_dynamic.hpp"
#include "jclassfile/model/constant/cp_class.hpp"
#include "jclassfile/model/constant/cp_double.hpp"
#include "jclassfile/model/constant/cp_dynamic.hpp"
#include "jclassfile/model/constant/cp_entry.hpp"
#include "jclassfile/model/constant/cp_field_ref.hpp"
#include "jclassfile/model/constant/cp_float.hpp"
#include "jclassfile/model/constant/cp_int.hpp"
#include "jclassfile/model/constant/cp_interface_method_ref.hpp"
#include "jclassfile/model/constant/cp_internal.hpp"
#include "jclassfile/model/constant/cp_invoke_dynamic.hpp"
#include "jclassfile/model/constant/cp_long.hpp"
#include "jclassfile/model/constant/cp_method_handle.hpp"
#include "jclassfile/model/constant/cp_method_ref.hpp"
#include "jclassfile/model/constant/cp_method_type.hpp"
#include "jclassfile/model/constant/cp_module.hpp"
#include "jclassfile/model/constant/cp_name_type.hpp"
#include "jclassfile/model/constant/cp_package.hpp"
#include "jclassfile/model/constant/cp_string.hpp"
#include "jclassfile/model/constant/cp_utf8.hpp"
#include "jclassfile/model/constant/cross_cp_referencing.hpp"
#include "jclassfile/model/constant/loadable_constant.hpp"
#include "jclassfile/model/constant/placeholders.hpp"

// model/instruction/ — 指令模型
#include "jclassfile/model/instruction/instruction.hpp"
#include "jclassfile/model/instruction/opcodes.hpp"
#include "jclassfile/model/instruction/opcode_names.hpp"
#include "jclassfile/model/instruction/reserved_opcodes.hpp"
#include "jclassfile/model/instruction/basic_instruction.hpp"
#include "jclassfile/model/instruction/cp_ref_instruction.hpp"
#include "jclassfile/model/instruction/iinc_instruction.hpp"
#include "jclassfile/model/instruction/int_operand_instruction.hpp"
#include "jclassfile/model/instruction/lookup_switch_instruction.hpp"
#include "jclassfile/model/instruction/multi_a_new_array_instruction.hpp"
#include "jclassfile/model/instruction/table_switch_instruction.hpp"
#include "jclassfile/model/instruction/wide_instruction.hpp"

// ============================================================================
// io/ — 读写器
// ============================================================================

#include "jclassfile/io/class_builder.hpp"
#include "jclassfile/io/class_file_reader.hpp"
#include "jclassfile/io/class_file_writer.hpp"
#include "jclassfile/io/attribute_reader.hpp"
#include "jclassfile/io/attribute_writer.hpp"
#include "jclassfile/io/attribute_context.hpp"
#include "jclassfile/io/attribute_holder_type.hpp"
#include "jclassfile/io/annotation_reader.hpp"
#include "jclassfile/io/annotation_writer.hpp"
#include "jclassfile/io/instruction_reader.hpp"
#include "jclassfile/io/instruction_writer.hpp"
#include "jclassfile/io/fallback_instruction_reader.hpp"
#include "jclassfile/io/fallback_instruction_writer.hpp"
#include "jclassfile/io/indexable_byte_stream.hpp"

// ============================================================================
// transform/ — 变换器
// ============================================================================

#include "jclassfile/transform/transformer.hpp"
#include "jclassfile/transform/illegal_stripping_transformer.hpp"
#include "jclassfile/transform/illegal_rewriting_instructions_reader.hpp"

// ============================================================================
// util/ — 工具
// ============================================================================

#include "jclassfile/util/growing_byte_buffer.hpp"
#include "jclassfile/util/opcode_util.hpp"

// ============================================================================
// 顶层异常
// ============================================================================

#include "jclassfile/invalid_class_exception.hpp"
