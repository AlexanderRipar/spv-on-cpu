#ifndef SPVCPU_HPP_INCLUDE_GUARD
#define SPVCPU_HPP_INCLUDE_GUARD

#include <cstdint>

#include "spv_result.hpp"

namespace spvcpu
{
	__declspec(dllexport) result show_spirv(
		uint32_t shader_bytes,
		const void* shader_data,
		const void* spirv_data,
		char** out_disassembly
	) noexcept;
}

#endif // SPVCPU_HPP_INCLUDE_GUARD
