#ifndef SPIRD_NAMES_HPP_INCLUDE_GUARD
#define SPIRD_NAMES_HPP_INCLUDE_GUARD

#include "spird_defs.hpp"

namespace spird
{
	bool get_name_from_arg_type(spird::arg_type type, const char** out_name) noexcept;

	bool get_arg_type_from_name(const char* name, uint32_t bytes, spird::arg_type* out_type) noexcept;
}

#endif // SPIRD_NAMES_HPP_INCLUDE_GUARD
