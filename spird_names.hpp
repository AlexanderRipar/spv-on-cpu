#ifndef SPIRD_NAMES_HPP_INCLUDE_GUARD
#define SPIRD_NAMES_HPP_INCLUDE_GUARD

#include "spird_defs.hpp"

namespace spird
{
	bool get_name_from_arg_type(spird::arg_type type, const char** out_name) noexcept;

	bool get_arg_type_from_name(const char* name, uint32_t bytes, spird::arg_type* out_type) noexcept;

	bool get_name_from_capability_id(uint16_t id, const char** out_name) noexcept;

	bool get_capability_id_from_name(const char* name, uint32_t bytes, uint16_t* out_id) noexcept;

	bool get_name_from_enum_id(spird::enum_id id, const char** out_name) noexcept;

	bool get_enum_id_from_name(const char* name, uint32_t bytes, spird::enum_id* out_id) noexcept;
}

#endif // SPIRD_NAMES_HPP_INCLUDE_GUARD
