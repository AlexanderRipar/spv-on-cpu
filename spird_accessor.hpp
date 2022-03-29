#ifndef SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD
#define SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD

#include <cstdint>

#include "spird_defs.hpp"
#include "spv_result.hpp"

namespace spird
{
	enum class implies_or_depends_mode : uint8_t
	{
		none = 0,
		depends = 1,
		implies = 2,
	};

	struct data_info
	{
		const char* name;

		uint32_t argc;

		spird::enum_flags enum_flags;

		spird::arg_type arg_types[256];

		const char* arg_names[256];
		
		implies_or_depends_mode implies_or_depends;

		uint8_t capability_cnt;

		uint16_t capabilities[127];
	};
	
	spvcpu::result get_data(const void* spv_data, spird::enum_id enum_id, uint32_t id, spird::data_info* out_data) noexcept;
}

#endif // SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD
