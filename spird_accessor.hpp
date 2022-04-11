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

	struct enum_location
	{
		spird::table_header m_table_header;

		uint32_t m_name_beg;

		uint32_t m_table_header_beg;
	};

	struct elem_data
	{
		const char* name;

		uint32_t argc;

		spird::arg_type arg_types[256];

		spird::arg_flags arg_flags[256];

		const char* arg_names[256];
		
		implies_or_depends_mode implies_or_depends;

		uint8_t capability_cnt;

		uint16_t capabilities[127];
	};

	struct enum_data
	{
		const char* name;

		spird::enum_flags flags;
	};
	
	spvcpu::result get_enum_location(const void* spird, enum_id enum_id, enum_location* out_location) noexcept;

	spvcpu::result get_enum_location(const void* spird, const char* enum_name, enum_location* out_location) noexcept;

	spvcpu::result get_elem_data(const void* spird, const enum_location& location, uint32_t elem_id, elem_data* out_data) noexcept;

	spvcpu::result get_enum_data(const void* spird, const enum_location& location, enum_data* out_data) noexcept;
}

#endif // SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD
