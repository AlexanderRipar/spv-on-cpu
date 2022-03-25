#ifndef SPVCPU_HPP_INCLUDE_GUARD
#define SPVCPU_HPP_INCLUDE_GUARD

#include <vulkan/vulkan.h>

namespace spvcpu
{
	struct shader_handle
	{
		void* m_data;
	};

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

	struct shader_create_info
	{
		VkShaderStageFlags stage;
		uint32_t shader_bytes;
		const void* shader_data;
		uint32_t entry_point_cnt;
		const char** entry_points;
		VkSpecializationInfo spec_info;
	};

	__declspec(dllexport) result load_shader(
		const shader_create_info* create_info,
		shader_handle* out_handle
	) noexcept;

	__declspec(dllexport) result show_spirv(
		uint32_t shader_bytes,
		const void* shader_data,
		char** out_disassembly
	) noexcept;
}

#endif // SPVCPU_HPP_INCLUDE_GUARD
