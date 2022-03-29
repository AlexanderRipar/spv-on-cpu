#ifndef SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD
#define SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD

#include <cstdint>

#include "spird_defs.hpp"
#include "spv_result.hpp"

namespace spird
{
	struct data_info
	{
		const char* name;

		uint32_t argc;

		spird::insn_argtype arg_types[256];

		const char* arg_names[256];
	};
}

spvcpu::result get_spirv_data(const void* spv_data, spird::enum_id enum_id, uint32_t id, spird::data_info* out_data) noexcept;

#endif // SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD
