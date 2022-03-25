#include "spv_data_accessor.hpp"

#include "spv_data_hashing.hpp"

#include <cstring>

uint32_t get_operation_data(const void* spv_data, uint32_t opcode, const char** op_name, uint32_t max_argc, spirv_insn_argtype* arg_types, const char** arg_names) noexcept
{
	const spirv_data_header* header = static_cast<const spirv_data_header*>(spv_data);

	const spirv_insn_index* table = reinterpret_cast<const spirv_insn_index*>(static_cast<const uint8_t*>(spv_data) + sizeof(spirv_data_header));

	const char* data = reinterpret_cast<const char*>(table) + header->table_size * sizeof(spirv_insn_index);

	uint32_t hash = hash_knuth(opcode, header->table_size);

	while (table[hash].opcode != opcode)
	{
		++hash;

		if (hash >= header->table_size)
			return ~0u;
	}

	uint32_t offset = table[hash].byte_offset;

	const char* entry = data + offset;

	uint32_t argc = static_cast<uint32_t>(*entry);

	if (argc > max_argc)
		return ~0u;

	*op_name = reinterpret_cast<const char*>(entry + 1);

	for(uint32_t i = 0; i != argc; ++i)
	{
		entry += strlen(entry + 1) + 2;

		arg_types[i] = static_cast<spirv_insn_argtype>(*entry);

		arg_names[i] = entry + 1;
	}

	return argc;
}
