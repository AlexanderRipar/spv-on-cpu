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
		unknown_argtype,
		id_arg_without_id,
	};
}

#endif // SPV_RESULT_HPP_INCLUDE_GUARD
