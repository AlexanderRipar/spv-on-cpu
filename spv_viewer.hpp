#ifndef SPVCPU_HPP_INCLUDE_GUARD
#define SPVCPU_HPP_INCLUDE_GUARD

#include <cstdint>

#include "spv_result.hpp"

namespace spvcpu
{
	__declspec(dllexport) result show_spirv(
		uint64_t spirv_bytes,
		const void* spirv,
		const void* spird,
		char** out_disassembly
	) noexcept;
}

#endif // SPVCPU_HPP_INCLUDE_GUARD
