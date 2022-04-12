#ifndef SPV_RESULT_HPP_INCLUDE_GUARD
#define SPV_RESULT_HPP_INCLUDE_GUARD

namespace spvcpu
{
	enum class result
	{
		success,
		no_memory,
		wrong_endianness,
		wrong_magic,
		invalid_spirv_version,
		too_many_ids,
		cannot_handle_header_schema,
		shader_too_small,
		shader_size_not_divisible_by_four,
		shader_data_misaligned,
		instruction_past_data_end,
		unhandled_opcode,
		instruction_wordcount_mismatch,
		unknown_opcode,
		too_many_instruction_args,
		spirv_data_unknown_version,
		spirv_data_enumeration_not_found,
		spirv_data_too_many_names,
		unknown_argtype,
		id_arg_without_id,
		id_not_found,
		unknown_rsttype,
		incompatible_types,
		untyped_result,
		unexpected_literal_type,
		unhandled_float_width,
		expected_constant,
		unknown_constant_instruction,
	};
}

#endif // SPV_RESULT_HPP_INCLUDE_GUARD
