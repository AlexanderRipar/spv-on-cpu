#include "spv_viewer.hpp"

__declspec(dllexport) spvcpu::result spvcpu::show_spirv(
	uint32_t shader_bytes,
	const void* shader_data,
	const void* spirv_data,
	char** out_disassembly
) noexcept
{
	return result::success;
}