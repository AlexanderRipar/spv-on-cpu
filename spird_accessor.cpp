#include "spird_accessor.hpp"

#include "spird_hashing.hpp"

#include <cstring>

spvcpu::result spird::get_data(const void* spv_data, spird::enum_id enum_id, uint32_t id, spird::data_info* out_data) noexcept
{
	const uint32_t enum_id_uint = static_cast<uint32_t>(enum_id);

	const uint8_t* raw_data = static_cast<const uint8_t*>(spv_data);

	const spird::file_header* file_header = static_cast<const spird::file_header*>(spv_data);

	if (file_header->version != 3)
		return spvcpu::result::spirv_data_unknown_version;

	if (enum_id_uint > file_header->table_count)
		return spvcpu::result::spirv_data_enumeration_not_found;

	const spird::table_header* instruction_table_header = reinterpret_cast<const spird::table_header*>(raw_data + sizeof(spird::file_header)) + enum_id_uint;

	const spird::insn_index* instruction_table = reinterpret_cast<const spird::insn_index*>(raw_data + instruction_table_header->table_offset);

	uint32_t hash = hash_knuth(id, instruction_table_header->size());

	while (instruction_table[hash].opcode != id)
	{
		++hash;

		if (hash >= instruction_table_header->size())
			return spvcpu::result::unknown_opcode;
	}

	uint32_t offset = instruction_table[hash].byte_offset;

	spird::info_type_mask info_types = instruction_table_header->types();

	const char* entry = reinterpret_cast<const char*>(raw_data + offset);

	uint32_t argc = 0;

	if ((info_types & spird::info_type_mask::arg_all_) != spird::info_type_mask::none)
		argc = static_cast<uint32_t>(*entry++);

	if (argc > 256)
		return spvcpu::result::too_many_instruction_args;

	if ((info_types & spird::info_type_mask::name) != spird::info_type_mask::none)
	{
			out_data->name = reinterpret_cast<const char*>(entry);

			entry += strlen(entry) + 1;
	}
	else
		out_data->name = nullptr;

	out_data->argc = argc;

	for (uint32_t i = 0; i != argc; ++i)
	{
		if ((info_types & spird::info_type_mask::arg_type) != spird::info_type_mask::none)
			out_data->arg_types[i] = static_cast<spird::insn_argtype>(*entry++);
		else
			out_data->arg_types[i] = spird::insn_argtype::UNKNOWN;

		if ((info_types & spird::info_type_mask::arg_all_) == spird::info_type_mask::arg_name)
		{
			out_data->arg_names[i] = entry[0] == '\0' ? nullptr : entry;

			entry += strlen(entry) + 1;
		}
		else
		{
			out_data->arg_names[i] = nullptr;
		}
	}

	return spvcpu::result::success;
}
