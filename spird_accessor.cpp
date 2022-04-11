#include "spird_accessor.hpp"

#include "spird_hashing.hpp"

#include <cstring>

static constexpr const char* const enum_name_strings[]
{
	"Instruction",
	"SourceLanguage",
	"ExecutionModel",
	"AddressingModel",
	"MemoryModel",
	"ExecutionMode",
	"StorageClass",
	"Dim",
	"SamplerAddressingMode",
	"SamplerFilterMode",
	"ImageFormat",
	"ImageChannelOrder",
	"ImageChannelDataType",
	"ImageOperands",
	"FpFastMathMode",
	"FpRoundingMode",
	"LinkageType",
	"AccessQualifier",
	"FunctionParameterAttribute",
	"Decoration",
	"Builtin",
	"SelectionControl",
	"LoopControl",
	"FunctionControl",
	"MemorySemantics",
	"MemoryOperands",
	"Scope",
	"GroupOperation",
	"KernelEnqueueFlags",
	"KernelProfilingInfo",
	"Capability",
	"ReservedRayFlags",
	"ReservedRayQueryIntersection",
	"ReservedRayQueryCommittedType",
	"ReservedRayQueryCandidateType",
	"ReservedFragmentShadingRate",
	"ReservedFpDenormMode",
	"ReservedFpOperationMode",
	"QuantizationMode",
	"OverflowMode",
	"PackedVectorFormat",
};

spvcpu::result spird::get_enum_location(const void* spird, spird::enum_id enum_id, spird::enum_location* out_location) noexcept
{
	const uint8_t* raw_data = static_cast<const uint8_t*>(spird);

	const spird::file_header* file_header = static_cast<const spird::file_header*>(spird);

	if (file_header->version != 17)
		return spvcpu::result::spirv_data_unknown_version;

	if (static_cast<uint32_t>(enum_id) >= file_header->unnamed_table_count)
		return spvcpu::result::spirv_data_enumeration_not_found;

	out_location->m_name_beg = 0;

	out_location->m_table_header_beg = file_header->first_table_header_byte + static_cast<uint32_t>(enum_id) * sizeof(spird::table_header);

	out_location->m_table_header = *reinterpret_cast<const spird::table_header*>(raw_data + out_location->m_table_header_beg);

	return spvcpu::result::success;
}

spvcpu::result spird::get_enum_location(const void* spird, const char* enum_name, spird::enum_location* out_location) noexcept
{
	const uint8_t* raw_data = static_cast<const uint8_t*>(spird);

	const spird::file_header* file_header = static_cast<const spird::file_header*>(spird);

	if (file_header->version != 17)
		return spvcpu::result::spirv_data_unknown_version;

	const char* curr_name = static_cast<const char*>(spird) + sizeof(spird::file_header);

	uint32_t enum_index = 0;

	while (curr_name < static_cast<const char*>(spird) + file_header->first_table_header_byte)
	{
		if (strcmp(enum_name, curr_name) == 0)
			goto FOUND;

		++enum_index;

		curr_name += strlen(curr_name) + 1;
	}

	return spvcpu::result::spirv_data_enumeration_not_found;

	FOUND:

	out_location->m_name_beg = curr_name - static_cast<const char*>(spird);

	out_location->m_table_header_beg = file_header->first_table_header_byte + (file_header->unnamed_table_count + enum_index) * sizeof(spird::table_header);

	out_location->m_table_header = *reinterpret_cast<const spird::table_header*>(raw_data + out_location->m_table_header_beg);

	return spvcpu::result::success;
}

spvcpu::result spird::get_elem_data(const void* spird, const spird::enum_location& location, uint32_t id, spird::elem_data* out_data) noexcept
{
	const uint8_t* raw_data = static_cast<const uint8_t*>(spird);

	const spird::elem_index* table = reinterpret_cast<const spird::elem_index*>(raw_data + location.m_table_header.offset);

	uint32_t hash = hash_knuth(id, location.m_table_header.size);

	const uint32_t initial_hash = hash;

	while (table[hash].id != id)
	{
		++hash;

		if (hash >= location.m_table_header.size)
			hash -= location.m_table_header.size;

		if (hash == initial_hash)
			return spvcpu::result::unknown_opcode;
	}

	uint32_t offset = table[hash].byte_offset;

	const char* entry = reinterpret_cast<const char*>(raw_data + offset);

	uint32_t argc = static_cast<uint32_t>(*entry++);

	if (argc > 256)
		return spvcpu::result::too_many_instruction_args;

	out_data->name = reinterpret_cast<const char*>(entry);

	entry += strlen(entry) + 1;

	out_data->argc = argc;

	for (uint32_t i = 0; i != argc; ++i)
	{
		out_data->arg_flags[i] = static_cast<spird::arg_flags>(*entry++);

		out_data->arg_types[i] = static_cast<spird::arg_type>(*entry++);

		out_data->arg_names[i] = entry[0] == '\0' ? nullptr : entry;

		entry += strlen(entry) + 1;
	}

	uint8_t cnt = *entry++;

	if (cnt == 0)
	{
		out_data->implies_or_depends = implies_or_depends_mode::none;
	}
	else
	{
		out_data->implies_or_depends = cnt & 0x80 ? implies_or_depends_mode::implies : implies_or_depends_mode::depends;

		cnt &= 0x7F;

		out_data->capability_cnt = cnt;

		for (uint8_t i = 0; i != cnt; ++i)
		{
			uint16_t lo = static_cast<uint8_t>(*entry++);

			uint16_t hi = static_cast<uint8_t>(*entry++);

			out_data->capabilities[i] = lo | (hi << 8);
		}
	}

	return spvcpu::result::success;
}

spvcpu::result spird::get_enum_data(const void* spird, const spird::enum_location& location, spird::enum_data* out_data) noexcept
{
	if (location.m_name_beg != 0)
		out_data->name = static_cast<const char*>(spird) + location.m_name_beg;
	else
	{
		const spird::file_header* file_header = static_cast<const spird::file_header*>(spird);

		const uint32_t enum_id = (location.m_table_header_beg - file_header->first_table_header_byte) / sizeof(spird::table_header);

		if (enum_id < file_header->unnamed_table_count)
			out_data->name = enum_name_strings[enum_id];
		else
			out_data->name = "<Unknown>";

	}
	
	out_data->flags = location.m_table_header.flags;

	return spvcpu::result::success;
}

spvcpu::result spird::get_named_enum_data(const void* spird, spird::named_enum_data* out_data) noexcept
{
	const spird::file_header* file_header = static_cast<const spird::file_header*>(spird);

	const char* curr_name = static_cast<const char*>(spird) + sizeof(spird::file_header);

	uint32_t name_count = 0;

	while (curr_name < static_cast<const char*>(spird) + file_header->first_table_header_byte)
	{
		if (name_count >= spird::max_named_enum_count)
			return spvcpu::result::spirv_data_too_many_names;

		if (*curr_name == '\0') // Reached padding; no more names
			break;

		out_data->names[name_count] = curr_name;

		curr_name += strlen(curr_name) + 1;

		++name_count;
	}

	out_data->name_count = name_count;

	return spvcpu::result::success;
}
