#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include "spv_viewer.hpp"
#include "spv_data_accessor.hpp"

static constexpr const char* argument_type_names[]{
	"ID",
	"STR",
	"U32",
	"I32",
	"F32",
	"RST",
	"RTYPE",
	"DECO",
	"LANGID",
	"TYPID",
	"MEMBER",
	"STRID",
	"DECOARG",
	"DECOARGID",
	"UNKNOWN",
	"ADDRMODEL",
	"MEMMODEL",
	"EXEMODEL",
	"EXEMODE",
	"CAPABILITY",
	"DIMENSION",
	"IMGFORMAT",
	"ACCESSQUAL",
	"STORAGECLASS",
	"SAMPADDRMODE",
	"SAMPFILTMODE",
	"OPCODE",
	"MEMOPERAND",
	"IMGOPERAND",
	"FUNCCTRL",
	"SELECTIONCTRL",
	"LOOPCTRL",
	"LOOPCTRLARG",
	"PACKEDVECFMT",
	"SCOPEID",
	"MEMSEMANTICID",
	"GROUPOP",
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

	uint32_t table_entry_cnt = reinterpret_cast<const spirv_data_table_header*>(static_cast<const uint8_t*>(spv_data) + sizeof(spirv_data_header))->table_size;

	fprintf(output_file, "instructions : [\r\n");

	const uint8_t* raw_data = static_cast<const uint8_t*>(spv_data);

	const spirv_data_header* file_header = static_cast<const spirv_data_header*>(spv_data);

	const spirv_insn_index* indices = reinterpret_cast<const spirv_insn_index*>(raw_data + sizeof(spirv_data_header) + sizeof(spirv_data_table_header) * file_header->table_count);

	for (uint32_t i = 0; i != table_entry_cnt; ++i)
	{
		uint32_t opcode = indices[i].opcode; 
		
		if (opcode == ~0u)
			continue;

		spirv_data_info op_data;

		if (spvcpu::result rst = get_spirv_data(spv_data, spirv_enum_id::Instruction, opcode, &op_data); rst != spvcpu::result::success)
		{
			printf("Could not get operation data for opcode %d. (Error %d)\n", opcode, rst);

			return 1;
		}

		fprintf(output_file, "\t{\r\n\t\topcode : %d\r\n\t\tname : \"%s\"\r\n\t\targs : [\n", opcode, op_data.name);

		for (uint32_t i = 0; i != op_data.argc; ++i)
		{
			uint8_t argtype = static_cast<uint8_t>(op_data.arg_types[i]);

			const char* optstr = "";

			const char* varstr = "";

			if (argtype & spirv_insn_arg_optional_bit)
				optstr = "OPT ";

			if (argtype & spirv_insn_arg_variadic_bit)
				varstr = "VAR ";

			argtype &= spirv_insn_argtype_mask;

			const char* argtypename = "<Invalid>";

			if(argtype < _countof(argument_type_names))
				argtypename = argument_type_names[argtype];

			fprintf(output_file, "\t\t\t%s%s%s", optstr, varstr, argtypename);

			if (op_data.arg_names[i][0] == '\0')
				fprintf(output_file, "\r\n");
			else
				fprintf(output_file, " \"%s\"\n", op_data.arg_names[i]);
		}

		fprintf(output_file, "\t\t]\r\n\t}\n");
	}

	fprintf(output_file, "]\r\n");
}
