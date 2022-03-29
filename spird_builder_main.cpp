#include "spird_builder_strings.hpp"
#include "spird_defs.hpp"
#include "spird_hashing.hpp"

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#ifdef _WIN32
#define ftell _ftelli64
#define fseek _fseeki64
#endif

static const char* prog_name;

__declspec(noreturn) static void panic(const char* msg, ...) noexcept
{
	va_list vargs;

	va_start(vargs, msg);

	fprintf(stderr, "[%s] ", prog_name);

	vfprintf(stderr, msg, vargs);

	va_end(vargs);

	exit(1);
}

static constexpr const char* argument_type_names[]{
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

enum class pstate
{
	seek_enum_open,
	seek_insn_open,
	seek_insn_name,
	seek_insn_opcode,
	seek_insn_args,
	seek_args_name,
	seek_args_type,
	seek_insn_close,
	handle_enum_end,
};



static uint32_t instruction_index_count = 0;

static spird::insn_index instruction_indices[65536];

static spird::table_header table_headers[256];

static spird::insn_index* hashtables[256];

static void* enum_data[256];

static uint32_t enum_data_sizes[256];


struct output_data
{
private:

	uint8_t* m_data;
	uint32_t m_used;
	uint32_t m_capacity;

	void grow(uint32_t additional) noexcept
	{
		if (m_used + additional > m_capacity)
		{
			m_capacity *= 2;

			m_data = static_cast<uint8_t*>(realloc(m_data, m_capacity));

			if (m_data == nullptr)
				panic("realloc failed.\n");
		}
	}

public:

	output_data() noexcept : m_data{ static_cast<uint8_t*>(malloc(4096)) }, m_used{ 0 }, m_capacity{ 4096 } { if (m_data == nullptr) panic("malloc failed.\n"); }

	void* steal(uint32_t* bytes) noexcept
	{
		uint8_t* prev = m_data;

		m_data = static_cast<uint8_t*>(malloc(4096));

		*bytes = m_used;

		m_used = 0;

		m_capacity = 4096;

		if (m_data == nullptr)
			panic("malloc failed.\n");

		return prev;
	}

	void append(const char* str, uint32_t len) noexcept
	{
		grow(len + 1);

		memcpy(m_data + m_used, str, len);

		m_used += len + 1;

		m_data[m_used - 1] = '\0';
	}

	void append(spird::arg_type type) noexcept
	{
		grow(1);

		m_data[m_used++] = static_cast<uint8_t>(type);
	}

	void overwrite(uint32_t index, uint8_t data) noexcept
	{
		m_data[index] = data;
	}

	uint32_t reserve_byte() noexcept
	{
		grow(1);

		m_data[m_used] = '?';

		return m_used++;
	}

	uint32_t size() const noexcept
	{
		return m_used;
	}

	const uint8_t* data() const noexcept
	{
		return m_data;
	}
};

uint32_t line_number = 1;

static const char* isolate_token_for_panic(const char* str) noexcept
{
	static char buf[32];

	uint32_t i = 0;

	while (*str != ' ' && *str != '\t' && *str != '\r' && *str != '\n' && *str != '\0' && *str != '#')
	{
		if (i == sizeof(buf) - 1)
		{
			buf[sizeof(buf) - 1] = '\0';

			buf[sizeof(buf) - 2] = '.';

			buf[sizeof(buf) - 3] = '.';

			buf[sizeof(buf) - 4] = '.';

			return buf;
		}

		buf[i++] = *str++;
	}

	buf[i] = '\0';

	return buf;
}

__declspec(noreturn) static void parse_panic(const char* expected, const char* instead) noexcept
{
	panic("Line %d:Expected '%s'. Found '%s' instead.\n", line_number, expected, isolate_token_for_panic(instead));
}

static bool is_whitespace(char c) noexcept
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static const char* skip_whitespace(const char* str) noexcept
{
	while (is_whitespace(*str) || *str == '#')
	{
		if (*str == '#')
		{
			while (*str != '\n' && *str != '\r')
				++str;
		}

		if (*str == '\n')
			++line_number;

		if (*str == '\r')
		{
			++line_number;

			if (str[1] == '\n')
				++str;
		}

		++str;
	}

	return str;

}

static void create_hashtable(uint32_t* out_table_size, spird::insn_index** out_table, spird::info_type_mask curr_info_mask) noexcept
{
	uint32_t table_size = (instruction_index_count * 3) >> 1;

	if (table_size > 0xFF'FF'FF)
		panic("Size of table is greater than maximum of 0xFFFFFF.\n");

	spird::insn_index* table = static_cast<spird::insn_index*>(malloc(table_size * sizeof(spird::insn_index)));

	if (table == nullptr)
		panic("malloc failed.\n");

	memset(table, 0xFF, table_size * sizeof(spird::insn_index));

	uint16_t* offsets = static_cast<uint16_t*>(malloc(table_size * sizeof(uint16_t)));

	if (offsets == nullptr)
		panic("malloc failed.\n");

	memset(offsets, 0x00, table_size * sizeof(uint16_t));

	for (uint32_t i = 0; i != instruction_index_count; ++i)
	{
		spird::insn_index elem = instruction_indices[i];

		uint32_t hash = hash_knuth(elem.opcode, table_size);

		uint16_t offset = 0;

		while (table[hash].opcode != ~0u)
		{
			if (offset > offsets[hash])
			{
				uint16_t tmp_off = offsets[hash];

				offsets[hash] = offset;

				offset = tmp_off;

				spird::insn_index tmp_ind = table[hash];

				table[hash] = elem;

				elem = tmp_ind;
			}

			++hash;

			if (hash == table_size)
				hash = 0;

			++offset;
		}

		table[hash] = elem;

		offsets[hash] = offset;
	}

	*out_table_size = table_size | (static_cast<uint32_t>(curr_info_mask) << 24);

	*out_table = table;
}

static void print_usage() noexcept
{
	fprintf(stderr, "Usage: %s [--ignore=info[;info...]] inputfile outputfile\n", prog_name);
}

static bool parse_args(int argc, const char** argv, const char** out_input_filename, const char** out_output_filename, spird::info_type_mask* out_ignored_info_types)
{
	*out_input_filename = *out_output_filename = nullptr;

	*out_ignored_info_types = spird::info_type_mask::none;

	const char* input_filename = nullptr, * output_filename = nullptr;

	uint8_t ignore_mask = 0;

	if (argc < 3)
	{
		print_usage();

		if (argc == 2 && strcmp(argv[1], "--help") == 0)
			fputs(extended_help_string, stderr);
		else if (argc == 2 && strcmp(argv[1], "-h") == 0)
			fputs(basic_help_string, stderr);

		return false;
	}

	for (uint32_t i = 1; i != argc; ++i)
	{
		if (strncmp(argv[i], ignore_type_string, strlen(ignore_type_string)) == 0)
		{
			const char* str = argv[i] + strlen(ignore_type_string);

			while (*str != '\0')
			{
				uint32_t arg_len = 0;

				while (str[arg_len] != ':' && str[arg_len] != '\0')
					++arg_len;

				uint32_t ignore_idx = ~0u;

				for (uint32_t j = 0; j != _countof(ignore_type_name_strings); ++j)
					if (strncmp(str, ignore_type_name_strings[j], arg_len) == 0)
					{
						ignore_idx = j;

						break;
					}

				if (ignore_idx == ~0u)
				{
					fprintf(stderr, "Unexpected ignore type '%.*s'. Valid ignore types are:\n\t", arg_len, str);

					for (uint32_t j = 0; j != _countof(ignore_type_name_strings); ++j)
					{
						fprintf(stderr, "'%s'%s", ignore_type_name_strings[j], j == _countof(ignore_type_name_strings) - 1 ? "\n" : ", ");
					}

					fprintf(stderr, "More than one ignore type can be specified by separating types with ';'.\n");

					return false;
				}

				if (ignore_mask & (1 << ignore_idx))
				{
					fprintf(stderr, "ignore type '%s' specified more than once.\n", ignore_type_name_strings[ignore_idx]);

					return false;
				}

				ignore_mask |= 1 << ignore_idx;

				str += arg_len + (str[arg_len] == ':');
			}
		}
		else if (argv[i][0] == '-')
		{
			fprintf(stderr, "Unknown option '%s'.\n", argv[i]);

			print_usage();

			return false;
		}
		else if (input_filename == nullptr)
		{
			input_filename = argv[i];
		}
		else if (output_filename == nullptr)
		{
			output_filename = argv[i];
		}
		else
		{
			fprintf(stderr, "Too many arguments specified (Did not expect '%s').\n", argv[i]);

			print_usage();

			return false;
		}
	}

	*out_input_filename = input_filename;

	*out_output_filename = output_filename;

	*out_ignored_info_types = static_cast<spird::info_type_mask>(ignore_mask);

	return true;
}

int main(int argc, const char** argv)
{
	prog_name = argv[0];

	const char* input_filename, * output_filename;

	spird::info_type_mask ignored_info_types;

	if (!parse_args(argc, argv, &input_filename, &output_filename, &ignored_info_types))
		return 1;

	FILE* input_file;

	if (fopen_s(&input_file, argv[1], "rb") != 0)
		panic("Could not open file %s for reading.\n", argv[1]);

	if (fseek(input_file, 0, SEEK_END) != 0)
		panic("Could not seek to end of input file.\n");

	size_t input_bytes = ftell(input_file);

	if (input_bytes < 0)
		panic("Could not determine size of input file.\n");

	if (fseek(input_file, 0, SEEK_SET) != 0)
		panic("Could not seek to start of input file.\n");

	char* input = static_cast<char*>(malloc(static_cast<size_t>(input_bytes + 1)));

	if (input == nullptr)
		panic("malloc failed\n");

	size_t actual_bytes_read = fread(input, 1, input_bytes, input_file);

	fclose(input_file);

	if (actual_bytes_read != input_bytes)
		panic("Failed to read from file %s.", argv[1]);

	input[input_bytes] = '\0';

	// Find start array
	const char* curr = skip_whitespace(input);

	pstate state = pstate::seek_enum_open;

	spird::insn_index curr_index;

	uint8_t curr_argc;

	output_data output;

	bool prev_arg_was_optional;

	bool prev_arg_was_variadic;

	uint32_t curr_enum_type;

	spird::info_type_mask curr_info_mask;

	bool done = false;

	while (!done)
	{
		switch (state)
		{
		case pstate::seek_enum_open:
		{
			if (*curr == '\0')
			{
				done = true;

				break;
			}

			uint32_t enum_len = 0;

			while (!is_whitespace(curr[enum_len]))
				++enum_len;

			uint32_t table_index = ~0u;

			for (uint32_t i = 0; i != _countof(enum_name_strings); ++i)
			{
				if (strncmp(curr, enum_name_strings[i], enum_len) == 0 && strlen(enum_name_strings[i]) == enum_len)
				{
					table_index = i;

					break;
				}
			}

			curr_enum_type = table_index;

			curr_info_mask = spird::info_type_mask::none;

			if (table_index == ~0u)
				parse_panic("enum-name", curr);

			curr = skip_whitespace(curr + enum_len);

			if (*curr != ':')
				parse_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			if (*curr != '[')
				parse_panic("[", curr);

			curr = skip_whitespace(curr + 1);

			state = pstate::seek_insn_open;

			break;
		}
		case pstate::seek_insn_open:
		{
			if (*curr == '{')
			{
				if ((ignored_info_types & spird::info_type_mask::arg_all_) != spird::info_type_mask::arg_all_)
					curr_index.byte_offset = output.reserve_byte();
				else
					curr_index.byte_offset = output.size();

				state = pstate::seek_insn_opcode;
			}
			else if (*curr == ']')
			{
				state = pstate::handle_enum_end;
			}
			else
			{
				parse_panic("{|]", curr);
			}

			curr = skip_whitespace(curr + 1);

			break;
		}
		case pstate::seek_insn_opcode:
		{
			if (strncmp(curr, instruction_opcode_string, strlen(instruction_opcode_string)) != 0)
				parse_panic(instruction_opcode_string, curr);

			curr = skip_whitespace(curr + strlen(instruction_opcode_string));

			if (*curr != ':')
				parse_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			uint32_t opcode = 0;

			if (*curr < '0' || *curr > '9')
				parse_panic("[0-9]", curr);

			if (curr[1] == 'x')
			{
				curr += 2;

				while (true)
				{
					if (*curr >= '0' && *curr <= '9')
						opcode = opcode * 16 + *curr - '0';
					else if (*curr >= 'a' && *curr <= 'f')
						opcode = opcode * 16 + *curr - 'a';
					else if (*curr >= 'A' && *curr <= 'F')
						opcode = opcode * 16 + *curr - 'A';
					else
						break;

					++curr;
				}
			}
			else
			{
				while (*curr >= '0' && *curr <= '9')
					opcode = opcode * 10 + *(curr++) - '0';

			}

			curr_index.opcode = opcode;

			curr = skip_whitespace(curr);

			state = pstate::seek_insn_name;

			break;
		}
		case pstate::seek_insn_name:
		{
			if (strncmp(curr, instruction_name_string, strlen(instruction_name_string)) != 0)
				parse_panic(instruction_name_string, curr);

			curr = skip_whitespace(curr + strlen(instruction_name_string));

			if (*curr != ':')
				parse_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			if (*curr != '"')
				parse_panic("\"instruction-name\"", curr);

			++curr;

			uint32_t i = 0;

			while (curr[i] != '"')
			{
				if (curr[i] == '\0' || curr[i] == '\r' || curr[i] == '\n')
					parse_panic("\"instruction-name\"", curr);

				++i;
			}

			if ((ignored_info_types & spird::info_type_mask::name) == spird::info_type_mask::none)
			{
				output.append(curr, i);

				curr_info_mask |= spird::info_type_mask::name;
			}

			curr = skip_whitespace(curr + i + 1);

			state = pstate::seek_insn_args;

			break;
		}
		case pstate::seek_insn_args:
		{
			if (strncmp(curr, instruction_args_string, strlen(instruction_args_string)) == 0)
			{
				curr = skip_whitespace(curr + strlen(instruction_args_string));

				if (*curr != ':')
					parse_panic(":", curr);

				curr = skip_whitespace(curr + 1);

				if (*curr != '[')
					parse_panic("[", curr);

				curr_argc = 0;

				prev_arg_was_optional = false;

				prev_arg_was_variadic = false;

				curr = skip_whitespace(curr + 1);

				state = pstate::seek_args_type;
			}
			else
			{
				state = pstate::seek_insn_close;
			}

			break;
		}
		case pstate::seek_args_type:
		{
			if (*curr != ']')
			{
				if (prev_arg_was_variadic)
					panic("Line %d: Cannot have another argument after variadic argument.\n", line_number);

				uint32_t name_len;

				bool flag_optional = false;

				bool flag_variadic = false;

				bool is_flag = true;

				while (is_flag)
				{
					name_len = 0;

					while (!is_whitespace(curr[name_len]))
						++name_len;

					if (strncmp(curr, argument_optional_string, name_len) == 0 && argument_optional_string[name_len] == '\0')
					{
						curr = skip_whitespace(curr + name_len);

						prev_arg_was_optional = true;

						if (flag_optional)
							panic("Line %d: '%s' specified more than once.\n", line_number, argument_optional_string);

						flag_optional = true;

						is_flag = true;
					}
					else if (strncmp(curr, argument_variadic_string, name_len) == 0 && argument_variadic_string[name_len] == '\0')
					{
						curr = skip_whitespace(curr + name_len);

						prev_arg_was_variadic = true;

						if (flag_variadic)
							panic("Line %d: '%s' specified more than once.\n", line_number, argument_optional_string);

						flag_variadic = true;

						is_flag = true;
					}
					else
					{
						is_flag = false;
					}
				}

				if (!flag_optional && prev_arg_was_optional)
					panic("Line %d: Cannot have non-optional argument after optional argument.\n", line_number);

				uint32_t name_idx = ~0u;

				for (uint32_t i = 0; i != _countof(argument_type_names); ++i)
					if (strncmp(curr, argument_type_names[i], name_len) == 0 && argument_type_names[i][name_len] == '\0')
					{
						name_idx = i;

						uint8_t argument_type_and_flags = static_cast<uint8_t>(i);

						if (flag_optional)
							i |= spird::insn_arg_optional_bit;

						if (flag_variadic)
							i |= spird::insn_arg_variadic_bit;

						if ((ignored_info_types & spird::info_type_mask::arg_type) != spird::info_type_mask::arg_type)
						{
							output.append(static_cast<spird::arg_type>(argument_type_and_flags));

							curr_info_mask |= spird::info_type_mask::arg_type;
						}

						break;
					}

				++curr_argc;

				if (name_idx == ~0u)
					parse_panic("argument-type", curr);

				curr = skip_whitespace(curr + name_len);

				state = pstate::seek_args_name;
			}
			else
			{
				curr = skip_whitespace(curr + 1);

				state = pstate::seek_insn_close;
			}

			break;
		}
		case pstate::seek_args_name:
		{
			if (*curr == '"')
			{
				++curr;

				uint32_t i = 0;

				while (curr[i] != '"')
				{
					if (curr[i] == '\0' || curr[i] == '\r' || curr[i] == '\n')
						parse_panic("\"argument-name\"", curr);

					++i;
				}

				if ((ignored_info_types & spird::info_type_mask::arg_name) == spird::info_type_mask::none)
					output.append(curr, i);

				curr = skip_whitespace(curr + i + 1);
			}
			else
			{
				char c = '\0';

				if ((ignored_info_types & spird::info_type_mask::arg_name) != spird::info_type_mask::arg_name)
				{
					output.append(&c, 0);

					curr_info_mask |= spird::info_type_mask::arg_name;
				}
			}

			state = pstate::seek_args_type;

			break;
		}
		case pstate::seek_insn_close:
		{
			if (*curr != '}')
				parse_panic("}", curr);

			if ((ignored_info_types & spird::info_type_mask::arg_all_) != spird::info_type_mask::arg_all_)
				output.overwrite(curr_index.byte_offset, curr_argc);

			instruction_indices[instruction_index_count++] = curr_index;

			curr = skip_whitespace(curr + 1);

			state = pstate::seek_insn_open;

			break;
		}
		case pstate::handle_enum_end:
		{
			if (instruction_index_count != 0)
			{
				create_hashtable(&table_headers[curr_enum_type].table_size_and_types, &hashtables[curr_enum_type], curr_info_mask);

				uint32_t data_bytes;

				char* data = static_cast<char*>(output.steal(&data_bytes));

				// Remove argc if enum has no arguments and argc has not already been removed due to ignored types
				if ((curr_info_mask & spird::info_type_mask::arg_all_) == spird::info_type_mask::none && (ignored_info_types & spird::info_type_mask::arg_all_) != spird::info_type_mask::arg_all_)
				{
					for (uint32_t i = 0; i != instruction_index_count; ++i)
					{
						uint32_t beg = instruction_indices[i].byte_offset;

						uint32_t end = i == instruction_index_count - 1 ? beg + output.size() : instruction_indices[i + 1].byte_offset;

						for (uint32_t j = beg; j != end; ++j)
							data[j - i] = data[j];

						instruction_indices[i].byte_offset -= i;
					}

					data_bytes -= instruction_index_count;
				}

				enum_data[curr_enum_type] = data;

				enum_data_sizes[curr_enum_type] = data_bytes;

				instruction_index_count = 0;
			}

			state = pstate::seek_enum_open;

			break;
		}
		}
	}

	FILE* output_file;

	if (fopen_s(&output_file, argv[2], "wb") != 0)
		panic("Could not open file %s for writing.\n", argv[2]);

	uint32_t enum_count = 0;

	for (uint32_t i = _countof(table_headers); i != 0; --i)
		if (table_headers[i - 1].size() != 0)
		{
			enum_count = i;

			break;
		}

	spird::file_header file_header;
	file_header.version = 3;
	file_header.table_count = enum_count;

	if (fwrite(&file_header, 1, sizeof(file_header), output_file) != sizeof(file_header))
		panic("Could not write to file %s.\n", argv[2]);

	uint32_t table_offset = sizeof(file_header) + sizeof(table_headers[0]) * enum_count;

	for (uint32_t i = 0; i != enum_count; ++i)
	{
		if (table_headers[i].size() == 0)
			continue;

		table_headers[i].table_offset = table_offset;

		table_offset += table_headers[i].size() * sizeof(spird::insn_index);
	}

	if (fwrite(table_headers, 1, enum_count * sizeof(table_headers[0]), output_file) != enum_count * sizeof(table_headers[0]))
		panic("Could not write to file %s.\n", argv[2]);

	uint32_t enum_offset = table_offset;

	for (uint32_t i = 0; i != enum_count; ++i)
	{
		if (hashtables[i] == nullptr)
			continue;

		for (uint32_t j = 0; j != table_headers[i].size(); ++j)
			if (hashtables[i][j].opcode != ~0u)
				hashtables[i][j].byte_offset += enum_offset;

		enum_offset += enum_data_sizes[i];

		if (fwrite(hashtables[i], 1, table_headers[i].size() * sizeof(spird::insn_index), output_file) != table_headers[i].size() * sizeof(spird::insn_index))
			panic("Could not write to file %s.\n", argv[2]);
	}

	for (uint32_t i = 0; i != enum_count; ++i)
	{
		if (enum_data[i] == nullptr)
			continue;

		if (fwrite(enum_data[i], 1, enum_data_sizes[i], output_file) != enum_data_sizes[i])
			panic("Could not write to file %s.\n", argv[2]);
	}

	fclose(output_file);

	return 0;
}
