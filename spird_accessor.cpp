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

spvcpu::result spird::get_elem_data(const void* spv_data, spird::enum_id enum_id, uint32_t id, spird::elem_data* out_data) noexcept
{
	const uint32_t enum_id_uint = static_cast<uint32_t>(enum_id);

	const uint8_t* raw_data = static_cast<const uint8_t*>(spv_data);

	const spird::file_header* file_header = static_cast<const spird::file_header*>(spv_data);

	if (file_header->version < 10 || file_header->version > 15)
		return spvcpu::result::spirv_data_unknown_version;

	const uint32_t mode_bits = file_header->version - 10;

	const spird::data_mode mode = static_cast<spird::data_mode>(mode_bits >= 3 ? mode_bits - 3 : mode_bits);

	const bool has_implies_and_depends = mode_bits >= 3;

	if (enum_id_uint > file_header->table_count)
		return spvcpu::result::spirv_data_enumeration_not_found;

	const spird::table_header* table_header = reinterpret_cast<const spird::table_header*>(raw_data + sizeof(spird::file_header)) + enum_id_uint;

	const spird::elem_index* table = reinterpret_cast<const spird::elem_index*>(raw_data + table_header->offset);

	uint32_t hash = hash_knuth(id, table_header->size);

	const uint32_t initial_hash = hash;

	while (table[hash].id != id)
	{
		++hash;

		if (hash >= table_header->size)
			hash -= table_header->size;

		if (hash == initial_hash)
			return spvcpu::result::unknown_opcode;
	}

	uint32_t offset = table[hash].byte_offset;

	const char* entry = reinterpret_cast<const char*>(raw_data + offset);

	uint32_t argc = static_cast<uint32_t>(*entry++);

	if (argc > 256)
		return spvcpu::result::too_many_instruction_args;

	if (mode == spird::data_mode::all || mode == spird::data_mode::disassembly)
	{
		out_data->name = reinterpret_cast<const char*>(entry);

		entry += strlen(entry) + 1;
	}
	else
	{
		out_data->name = nullptr;
	}

	out_data->argc = argc;

	for (uint32_t i = 0; i != argc; ++i)
	{
		out_data->arg_flags[i] = static_cast<spird::arg_flags>(*entry++);

		out_data->arg_types[i] = static_cast<spird::arg_type>(*entry++);

		if (mode == spird::data_mode::all || mode == spird::data_mode::disassembly)
		{
			out_data->arg_names[i] = entry[0] == '\0' ? nullptr : entry;

			entry += strlen(entry) + 1;
		}
		else
		{
			out_data->arg_names[i] = nullptr;
		}
	}

	if (has_implies_and_depends)
	{
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
	}
	else
	{
		out_data->implies_or_depends = implies_or_depends_mode::none;
	}

	return spvcpu::result::success;
}

spvcpu::result spird::get_enum_data(const void* spv_data, spird::enum_id enum_id, spird::enum_data* out_data) noexcept
{
	const uint32_t enum_id_uint = static_cast<uint32_t>(enum_id);

	const uint8_t* raw_data = static_cast<const uint8_t*>(spv_data);

	const spird::file_header* file_header = static_cast<const spird::file_header*>(spv_data);

	if (file_header->version < 4 || file_header->version > 15)
		return spvcpu::result::spirv_data_unknown_version;

	if (enum_id_uint > file_header->table_count)
		return spvcpu::result::spirv_data_enumeration_not_found;

	const spird::table_header* table_header = reinterpret_cast<const spird::table_header*>(raw_data + sizeof(spird::file_header)) + enum_id_uint;

	if (enum_id_uint > spird::enum_id_count)
		out_data->name = "<Unknown>";
	else
		out_data->name = enum_name_strings[enum_id_uint];

	out_data->header = table_header;

	return spvcpu::result::success;
}