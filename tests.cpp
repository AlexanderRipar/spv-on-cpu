#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include "spird_accessor.hpp"

static constexpr const char* argument_type_names[]
{
	"INSTRUCTION",
	"SOURCELANGUAGE",
	"EXECUTIONMODEL",
	"ADDRESSINGMODEL",
	"MEMORYMODEL",
	"EXECUTIONMODE",
	"STORAGECLASS",
	"DIM",
	"SAMPLERADDRESSINGMODE",
	"SAMPLERFILTERMODE",
	"IMAGEFORMAT",
	"IMAGECHANNELORDER",
	"IMAGECHANNELDATATYPE",
	"IMAGEOPERANDS",
	"FPFASTMATHMODE",
	"FPROUNDINGMODE",
	"LINKAGETYPE",
	"ACCESSQUALIFIER",
	"FUNCTIONPARAMETERATTRIBUTE",
	"DECORATION",
	"BUILTIN",
	"SELECTIONCONTROL",
	"LOOPCONTROL",
	"FUNCTIONCONTROL",
	"MEMORYSEMANTICSID",
	"MEMORYOPERANDS",
	"SCOPEID",
	"GROUPOPERATION",
	"KERNELENQUEUEFLAGS",
	"KERNELPROFILINGINFO",
	"CAPABILITY",
	"RESERVEDRAYFLAGS",
	"RESERVEDRAYQUERYINTERSECTION",
	"RESERVEDRAYQUERYCOMMITTEDTYPE",
	"RESERVEDRAYQUERYCANDIDATETYPE",
	"RESERVEDFRAGMENTSHADINGRATE",
	"RESERVEDFPDENORMMODE",
	"RESERVEDFPOPERATIONMODE",
	"QUANTIZATIONMODE",
	"OVERFLOWMODE",
	"PACKEDVECTORFORMAT",

	"RST",
	"RTYPE",
	"LITERAL",
	"ID",
	"TYPID",
	"U32",
	"STR",
	"ARG",
	"MEMBER",
	"U32IDPAIR",
	"IDMEMBERPAIR",
	"IDIDPAIR",
	"IDU32PAIR",
	"I64",
};

int main(int argc, const char** argv)
{
	if (argc != 2 && argc != 3)
	{
		printf("Usage: %s input-file [output-file]\n", argv[0]);
	}

	FILE* input_file;

	FILE* output_file = stdout;

	if (fopen_s(&input_file, argv[1], "rb") != 0)
	{
		printf("Could not open file %s for reading.\n", argv[1]);

		return 1;
	}

	if (fseek(input_file, 0, SEEK_END) != 0)
	{
		printf("Could not seek in file.\n");

		return 1;
	}

	size_t input_bytes = ftell(input_file);

	if (input_bytes < 0)
	{
		printf("Could not ftell file.\n");

		return 1;
	}

	if (fseek(input_file, 0, SEEK_SET) != 0)
	{
		printf("Could not seek in file.\n");

		return 1;
	}

	void* spv_data = malloc(input_bytes);

	if (fread(spv_data, 1, input_bytes, input_file) != input_bytes)
	{
		printf("Could not read from file.\n");

		return 1;
	}

	if (argc == 3)
	{
		if (fopen_s(&output_file, argv[2], "wb") != 0)
		{
			printf("Could not open file %s for writing.\n", argv[2]);

			return 1;
		}
	}

	uint32_t table_entry_cnt = reinterpret_cast<const spird::table_header*>(static_cast<const uint8_t*>(spv_data) + sizeof(spird::file_header))->size();

	fprintf(output_file, "instructions : [\n");

	const uint8_t* raw_data = static_cast<const uint8_t*>(spv_data);

	const spird::file_header* file_header = static_cast<const spird::file_header*>(spv_data);

	const spird::insn_index* indices = reinterpret_cast<const spird::insn_index*>(raw_data + sizeof(spird::file_header) + sizeof(spird::table_header) * file_header->table_count);

	for (uint32_t i = 0; i != table_entry_cnt; ++i)
	{
		uint32_t opcode = indices[i].opcode;

		if (opcode == ~0u)
			continue;

		spird::data_info op_data;

		if (spvcpu::result rst = get_spirv_data(spv_data, spird::enum_id::Instruction, opcode, &op_data); rst != spvcpu::result::success)
		{
			printf("Could not get operation data for opcode %d. (Error %d)\n", opcode, rst);

			return 1;
		}

		fprintf(output_file, "\t{\n\t\topcode : %d\n\t\tname : \"%s\"\n\t\targs : [\n", opcode, op_data.name == nullptr ? "<Unknown>" : op_data.name);

		for (uint32_t i = 0; i != op_data.argc; ++i)
		{
			uint8_t argtype = static_cast<uint8_t>(op_data.arg_types[i]);

			const char* optstr = "";

			const char* varstr = "";

			if (argtype & spird::insn_arg_optional_bit)
				optstr = "OPT ";

			if (argtype & spird::insn_arg_variadic_bit)
				varstr = "VAR ";

			argtype &= spird::insn_argtype_mask;

			const char* argtypename = "<Invalid>";

			if (argtype < _countof(argument_type_names))
				argtypename = argument_type_names[argtype];

			fprintf(output_file, "\t\t\t%s%s%s", optstr, varstr, argtypename);

			if (op_data.arg_names[i] == nullptr)
				fprintf(output_file, "\n");
			else
				fprintf(output_file, " \"%s\"\n", op_data.arg_names[i]);
		}

		fprintf(output_file, "\t\t]\n\t}\n");
	}

	fprintf(output_file, "]\n");

	return 0;
}
