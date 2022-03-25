#ifndef SPVCPU_HPP_INCLUDE_GUARD
#define SPVCPU_HPP_INCLUDE_GUARD

#include <cstdint>

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
	};

	__declspec(dllexport) result show_spirv(
		uint32_t shader_bytes,
		const void* shader_data,
		const void* spirv_data,
		char** out_disassembly
	) noexcept;
}

#endif // SPVCPU_HPP_INCLUDE_GUARD
