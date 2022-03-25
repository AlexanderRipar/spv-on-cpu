#ifndef SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD
#define SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD

#include <cstdint>

#include "spv_data_defs.hpp"

uint32_t get_operation_data(const void* spv_data, uint32_t opcode, const char** op_name, uint32_t max_argc, spirv_insn_argtype* arg_types, const char** arg_names) noexcept;

#endif // SPV_DATA_ACCESSOR_HPP_INCLUDE_GUARD
