#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "spird_accessor.hpp"
#include "spird_names.hpp"
#include "spv_viewer.hpp"

#ifdef _WIN32
#define ftell _ftelli64
#define fseek _fseeki64
#endif

bool get_file_content(const char* filename, void** out_data, uint64_t* out_bytes) noexcept
{
	FILE* file;

	if (fopen_s(&file, filename, "rb") != 0)
	{
		fprintf(stderr, "Could not open file '%s'.\n", filename);

		return false;
	}

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fprintf(stderr, "Could not seek in file '%s'.\n", filename);

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	const int64_t bytes = ftell(file);

	if (bytes < 0)
	{
		fprintf(stderr, "Could not ftell in file '%s'.\n", filename);

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	if (fseek(file, 0, SEEK_SET) != 0)
	{
		fprintf(stderr, "Could not seek in file '%s'.\n", filename);

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	void* buffer = malloc(bytes);

	if (buffer == nullptr)
	{
		fprintf(stderr, "malloc failed.\n");

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	if (fread(buffer, 1, bytes, file) != bytes)
	{
		fprintf(stderr, "Could not read from file '%s'.\n", filename);

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	if (filename != nullptr)
		fclose(file);

	*out_data = buffer;

	*out_bytes = static_cast<uint64_t>(bytes);

	return true;
}

int cycle(int argc, const char** argv) noexcept
{
	if (argc != 2 && argc != 3)
	{
		fprintf(stderr, "Usage: %s input-file [output-file]\n", argv[0]);

		return 0;
	}

	void* spird;

	uint64_t spird_bytes;

	if (!get_file_content(argv[1], &spird, &spird_bytes))
		return 1;
		
	FILE* output_file = stdout;

	if (argc == 3)
	{
		if (fopen_s(&output_file, argv[2], "wb") != 0)
		{
			fprintf(stderr, "Could not open file %s for writing.\n", argv[2]);

			return 1;
		}
	}

	spird::named_enum_data named_data;

	if (spvcpu::result rst = spird::get_named_enum_data(spird, &named_data); rst != spvcpu::result::success)
	{
		fprintf(stderr, "Could not get data on named enumerations. (Error %d)\n", rst);

		return 1;
	}

	const uint32_t unnamed_table_cnt = reinterpret_cast<const spird::file_header*>(spird)->unnamed_table_count;

	const uint32_t named_table_cnt = named_data.name_count;

	// This works because the headers for unnamed and named tables are located directly after each other
	const uint32_t table_cnt = unnamed_table_cnt + named_table_cnt;

	for (uint32_t t = 0; t != table_cnt; ++t)
	{
		spird::enum_location enum_loc;

		if (t < unnamed_table_cnt)
		{
			if (spvcpu::result rst = spird::get_enum_location(spird, static_cast<spird::enum_id>(t), &enum_loc); rst != spvcpu::result::success)
			{
				fprintf(stderr, "Could not locate enumeration %d. (Error %d)\n", t, rst);

				return 1;
			}
		}
		else
		{
			if (spvcpu::result rst = spird::get_enum_location(spird, named_data.names[t - unnamed_table_cnt], &enum_loc); rst != spvcpu::result::success)
			{
				fprintf(stderr, "Could not locate enumeration %s. (Error %d)\n", named_data.names[t - unnamed_table_cnt], rst);

				return 1;
			}
		}

		spird::enum_data enum_data;

		if (spvcpu::result rst = spird::get_enum_data(spird, enum_loc, &enum_data); rst != spvcpu::result::success)
		{
			fprintf(stderr, "Could not get data for enumeration %d. (Error %d)\n", t, rst);

			return 1;
		}

		fprintf(output_file, "\n%s", enum_data.name);

		if ((enum_data.flags & spird::enum_flags::bitmask) == spird::enum_flags::bitmask)
			fprintf(output_file, "@BITMASK");

		if ((enum_data.flags & spird::enum_flags::named) == spird::enum_flags::named)
			fprintf(output_file, "@NAMED");

		fprintf(output_file, " : [\n");

		const uint8_t* raw_data = static_cast<const uint8_t*>(spird);

		const spird::file_header* file_header = static_cast<const spird::file_header*>(spird);

		const spird::table_header* table_header = reinterpret_cast<const spird::table_header*>(raw_data + file_header->first_table_header_byte) + t;

		const spird::elem_index* indices = reinterpret_cast<const spird::elem_index*>(raw_data + table_header->offset);

		for (uint32_t i = 0; i != table_header->size; ++i)
		{
			uint32_t id = indices[i].id;

			if (id == ~0u)
				continue;

			spird::elem_data elem_data;

			if (spvcpu::result rst = spird::get_elem_data(spird, enum_loc, id, &elem_data); rst != spvcpu::result::success)
			{
				fprintf(stderr, "Could not get data for element %d of enumeration '%s' (%d). (Error %d)\n", id, enum_data.name, t, rst);

				return 1;
			}

			if ((enum_data.flags & spird::enum_flags::bitmask) == spird::enum_flags::bitmask)
				fprintf(output_file, "\t{\n\t\tid : 0x%x", id);
			else
				fprintf(output_file, "\t{\n\t\tid : %d", id);

			fprintf(output_file, "\n\t\tname : \"%s\"\n", elem_data.name == nullptr ? "<Unknown>" : elem_data.name);

			if (elem_data.argc > 0)
				fprintf(output_file, "\t\targs : [\n");

			for (uint32_t i = 0; i != elem_data.argc; ++i)
			{
				spird::arg_type arg_type = elem_data.arg_types[i];

				spird::arg_flags arg_flags = elem_data.arg_flags[i];

				const char* optstr = "";

				const char* varstr = "";

				const char* idstr = "";

				const char* pairstr = "";

				if ((arg_flags & spird::arg_flags::optional) == spird::arg_flags::optional)
					optstr = "OPT ";

				if ((arg_flags & spird::arg_flags::variadic) == spird::arg_flags::optional)
					varstr = "VAR ";

				if ((arg_flags & spird::arg_flags::id) == spird::arg_flags::id)
					idstr = "ID ";

				if ((arg_flags & spird::arg_flags::pair) == spird::arg_flags::pair)
					pairstr = "PAIR ";

				const char* type_name;

				if (!spird::get_name_from_arg_type(arg_type, &type_name))
				{
					fprintf(stderr, "Could not get name of arg_type %d.\n", arg_type);

					return 1;
				}

				if ((arg_flags & spird::arg_flags::result) == spird::arg_flags::result)
				{
					fprintf(output_file, "\t\t\tRST %s\n", type_name);
				}
				else
				{
					fprintf(output_file, "\t\t\t%s%s%s%s", optstr, varstr, idstr, type_name);

					if (elem_data.arg_names[i] == nullptr)
						fprintf(output_file, "\n");
					else
						fprintf(output_file, " \"%s\"\n", elem_data.arg_names[i]);
				}
			}

			if (elem_data.argc > 0)
				fprintf(output_file, "\t\t]\n");

			if (elem_data.implies_or_depends != spird::implies_or_depends_mode::none)
			{
				fprintf(output_file, elem_data.implies_or_depends == spird::implies_or_depends_mode::depends ? "\t\tdepends : " : "\t\timplies : ");

				if (elem_data.capability_cnt > 1)
				{
					fprintf(output_file, "[\n");

					for (uint8_t i = 0; i != elem_data.capability_cnt; ++i)
					{
						const char* str;

						if (!spird::get_name_from_capability_id(elem_data.capabilities[i], &str))
						{
							fprintf(stderr, "Could not get name of capability %d.\n", elem_data.capabilities[i]);

							return 1;
						}

						fprintf(output_file, "\t\t\t%s\n", str);
					}

					fprintf(output_file, "\t\t]\n");
				}
				else
				{
					const char* str;

					if (!spird::get_name_from_capability_id(elem_data.capabilities[0], &str))
					{
						fprintf(stderr, "Could not get name of capability %d.\n", elem_data.capabilities[i]);

						return 1;
					}

					fprintf(output_file, "%s\n", str);
				}
			}

			fprintf(output_file, "\t}\n");
		}

		fprintf(output_file, "]\n");
	}

	return 0;
}

int disasm(int argc, const char** argv) noexcept
{
	if (argc < 3 || argc > 5)
	{
		printf("Usage: %s [--types] shader-file spird-file [output-file]\n", argv[0]);

		return 0;
	}

	bool print_type_info = false;

	uint32_t curr_arg = 1;

	if (strcmp(argv[1], "--types") == 0)
	{
		print_type_info = true;

		curr_arg = 2;
	}

	FILE* output_file = stdout;


	uint64_t shader_bytes;

	void* shader_data;

	uint64_t spird_bytes;

	void* spird_data;

	if (!get_file_content(argv[curr_arg++], &shader_data, &shader_bytes))
		return 1;

	if (!get_file_content(argv[curr_arg++], &spird_data, &spird_bytes))
		return 1;

	if (curr_arg != argc)
	{
		if (fopen_s(&output_file, argv[curr_arg], "w") != 0)
		{
			fprintf(stderr, "Could not open file %s for writing.\n", argv[2]);

			return 1;
		}
	}

	char* disassembly;

	uint64_t disassembly_bytes;

	if (spvcpu::result rst = spvcpu::disassemble(shader_bytes, shader_data, spird_data, print_type_info, &disassembly_bytes, &disassembly); rst != spvcpu::result::success)
	{
		fprintf(stderr, "spvcpu::disassemble failed with error %d.\n", static_cast<uint32_t>(rst));

		return 1;
	}

	if (fwrite(disassembly, 1, disassembly_bytes - 1, output_file) != disassembly_bytes - 1)
	{
		fprintf(stderr, "Could not write to %s%s", output_file == stdout ? "" : "file ", output_file == stdout ? "stdout" : argv[3]);

		return 1;
	}

	return 0;
}

void print_usage(const char* prog_name) noexcept
{
	fprintf(stderr, "Usage: %s (--cycle|--disasm) [additional args...]\n", prog_name);
}

int main(int argc, const char** argv)
{
	if (argc < 2)
	{
		print_usage(argv[0]);

		return 0;
	}
	else if (strcmp(argv[1], "--cycle") == 0)
	{
		return cycle(argc - 1, argv + 1);
	}
	else if (strcmp(argv[1], "--disasm") == 0)
	{
		return disasm(argc - 1, argv + 1);
	}
	else
	{
		print_usage(argv[0]);

		return 0;
	}

}
