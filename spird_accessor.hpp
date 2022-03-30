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

	struct elem_data
	{
		const char* name;

		uint32_t argc;

		spird::arg_type arg_types[256];

		const char* arg_names[256];
		
		implies_or_depends_mode implies_or_depends;

		uint8_t capability_cnt;

		uint16_t capabilities[127];
	};

	struct enum_data
	{
		const char* name;

		const spird::table_header* header;
	};
	
	spvcpu::result get_elem_data(const void* spv_data, spird::enum_id enum_id, uint32_t id, spird::elem_data* out_data) noexcept;

	spvcpu::result get_enum_data(const void* spv_data, spird::enum_id enum_id, spird::enum_data* out_data) noexcept;
}

#endif // SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD
