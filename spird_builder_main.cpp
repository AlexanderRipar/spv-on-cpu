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
	"VALUE",
	"TYPE",
	"UNKNOWN",
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
	enum_open,
	elem_open,
	elem_id,
	elem_name,
	args_open,
	args_type,
	args_name,
	elem_depends,
	elem_implies,
	elem_close,
	enum_close,
};



static uint32_t instruction_index_count = 0;

static spird::elem_index instruction_indices[65536];

static spird::table_header table_headers[256];

static spird::elem_index* hashtables[256];

static void* enum_data[256];

static uint32_t enum_data_sizes[256];

static uint64_t is_bitmask_enum[4];

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

	void append(uint16_t capability_id) noexcept
	{
		grow(2);

		m_data[m_used++] = static_cast<uint8_t>(capability_id);

		m_data[m_used++] = static_cast<uint8_t>(capability_id >> 8);
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

static void create_hashtable(uint16_t* out_table_size, spird::elem_index** out_table) noexcept
{
	uint32_t table_size = (instruction_index_count * 3) >> 1;

	if (table_size > 0xFFFF)
		panic("Size of table is greater than maximum of 0xFFFFFF.\n");

	spird::elem_index* table = static_cast<spird::elem_index*>(malloc(table_size * sizeof(spird::elem_index)));

	if (table == nullptr)
		panic("malloc failed.\n");

	memset(table, 0xFF, table_size * sizeof(spird::elem_index));

	uint16_t* offsets = static_cast<uint16_t*>(malloc(table_size * sizeof(uint16_t)));

	if (offsets == nullptr)
		panic("malloc failed.\n");

	memset(offsets, 0x00, table_size * sizeof(uint16_t));

	for (uint32_t i = 0; i != instruction_index_count; ++i)
	{
		spird::elem_index elem = instruction_indices[i];

		uint32_t hash = hash_knuth(elem.id, table_size);

		uint16_t offset = 0;

		while (table[hash].id != ~0u)
		{
			if (offset > offsets[hash])
			{
				uint16_t tmp_off = offsets[hash];

				offsets[hash] = offset;

				offset = tmp_off;

				spird::elem_index tmp_ind = table[hash];

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

	*out_table_size = table_size;

	*out_table = table;
}

static void print_usage() noexcept
{
	fprintf(stderr, "Usage: %s [--ignore=info[;info...]] inputfile outputfile\n", prog_name);
}

static bool parse_args(
	int argc, 
	const char** argv, 
	const char** out_input_filename, 
	const char** out_output_filename, 
	spird::data_mode* out_mode, 
	bool* out_no_implies_and_depends
)
{
	*out_input_filename = *out_output_filename = nullptr;

	*out_mode = spird::data_mode::all;

	*out_no_implies_and_depends = false;

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

	uint32_t mode_index = ~0u;

	bool no_implies_and_depends = false;

	for (uint32_t i = 1; i != argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			for (uint32_t j = 0; j != _countof(data_type_strings); ++j)
			{
				if (strcmp(argv[i], data_type_strings[j]) == 0)
				{
					if (mode_index != ~0u)
					{
						fprintf(stderr, "More than one of --for-[all|disassembly|debugging] specified.\n");

						return false;
					}

					mode_index = j;

					break;
				}
			}

			if (mode_index != ~0)
				continue;

			if (strcmp(argv[i], data_type_no_implies_and_depends_string))
			{
				if (no_implies_and_depends)
				{
					fprintf(stderr, "%s specified more than once.\n", data_type_no_implies_and_depends_string);

					return false;
				}

				no_implies_and_depends = true;
			}
			else
			{
				fprintf(stderr, "Unknown option '%s'.\n", argv[i]);

				print_usage();

				return false;
			}
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

	uint8_t mode = mode_index == ~0u ? static_cast<uint8_t>(spird::data_mode::all) : mode_index;

	if (no_implies_and_depends)
		*out_no_implies_and_depends = true;

	*out_mode = static_cast<spird::data_mode>(mode);

	return true;
}

static bool token_equal(const char*& curr, const char* token) noexcept
{
	uint32_t len = 0;

	while (!is_whitespace(curr[len]) && curr[len] != '\0' && curr[len] != ':' && curr[len] != enum_flag_char)
		++len;

	if (token[len] != '\0')
		return false;

	if (strncmp(curr, token, len) != 0)
		return false;

	curr = skip_whitespace(curr + len);

	return true;
}

int main(int argc, const char** argv)
{
	prog_name = argv[0];

	const char* input_filename, * output_filename;

	spird::data_mode mode;

	bool no_implies_and_depends;

	if (!parse_args(argc, argv, &input_filename, &output_filename, &mode, &no_implies_and_depends))
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

	pstate state = pstate::enum_open;

	spird::elem_index curr_index;

	uint8_t curr_argc;

	output_data output;

	bool prev_arg_was_optional;

	bool prev_arg_was_variadic;

	uint32_t curr_enum_type;

	bool done = false;

	while (!done)
	{
		switch (state)
		{
		case pstate::enum_open:
		{
			if (*curr == '\0')
			{
				done = true;

				break;
			}

			uint32_t table_index = ~0u;

			for (uint32_t i = 0; i != _countof(enum_name_strings); ++i)
				if (token_equal(curr, enum_name_strings[i]))
				{
					table_index = i;

					break;
				}

			if (table_index == ~0u)
				parse_panic("enum-name", curr);

			curr_enum_type = table_index;

			if (*curr == enum_flag_char)
			{
				++curr;

				if (!token_equal(curr, enum_flag_bitmask_string))
					parse_panic("BITMASK", curr);

				is_bitmask_enum[curr_enum_type >> 6] |= 1ui64 << (curr_enum_type & 63);
			}

			if (*curr != ':')
				parse_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			if (*curr != '[')
				parse_panic("[", curr);

			curr = skip_whitespace(curr + 1);

			state = pstate::elem_open;

			break;
		}
		case pstate::elem_open:
		{
			if (*curr == '{')
			{
				curr_index.byte_offset = output.reserve_byte();

				curr_argc = 0;

				prev_arg_was_optional = false;

				prev_arg_was_variadic = false;

				state = pstate::elem_id;
			}
			else if (*curr == ']')
			{
				state = pstate::enum_close;
			}
			else
			{
				parse_panic("{|]", curr);
			}

			curr = skip_whitespace(curr + 1);

			break;
		}
		case pstate::elem_id:
		{
			if (!token_equal(curr, elem_id_string))
				parse_panic(elem_id_string, curr);

			if (*curr != ':')
				parse_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			uint32_t id = 0;

			if (*curr < '0' || *curr > '9')
				parse_panic("[0-9]", curr);

			if (curr[1] == 'x')
			{
				curr += 2;

				while (true)
				{
					if (*curr >= '0' && *curr <= '9')
						id = id * 16 + *curr - '0';
					else if (*curr >= 'a' && *curr <= 'f')
						id = id * 16 + *curr - 'a';
					else if (*curr >= 'A' && *curr <= 'F')
						id = id * 16 + *curr - 'A';
					else
						break;

					++curr;
				}
			}
			else
			{
				while (*curr >= '0' && *curr <= '9')
					id = id * 10 + *(curr++) - '0';

			}

			curr_index.id = id;

			curr = skip_whitespace(curr);

			state = pstate::elem_name;

			break;
		}
		case pstate::elem_name:
		{
			if (!token_equal(curr, elem_name_string))
				parse_panic(elem_name_string, curr);

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

			if (mode == spird::data_mode::all || mode == spird::data_mode::disassembly)
				output.append(curr, i);

			curr = skip_whitespace(curr + i + 1);

			state = pstate::args_open;

			break;
		}
		case pstate::args_open:
		{
			if (token_equal(curr, elem_args_string))
			{
				if (*curr != ':')
					parse_panic(":", curr);

				curr = skip_whitespace(curr + 1);

				if (*curr != '[')
					parse_panic("[", curr);

				curr = skip_whitespace(curr + 1);

				state = pstate::args_type;
			}
			else
			{
				state = pstate::elem_depends;
			}

			break;
		}
		case pstate::args_type:
		{
			if (*curr != ']')
			{
				if (prev_arg_was_variadic)
					panic("Line %d: Cannot have another argument after variadic argument.\n", line_number);

				uint32_t name_len;

				bool flag_optional = false;

				bool flag_variadic = false;

				bool flag_id = false;

				while (true)
				{
					name_len = 0;

					while (!is_whitespace(curr[name_len]))
						++name_len;

					if (token_equal(curr, argument_optional_string))
					{
						prev_arg_was_optional = true;

						if (flag_optional)
							panic("Line %d: '%s' specified more than once.\n", line_number, argument_optional_string);

						flag_optional = true;
					}
					else if (token_equal(curr, argument_variadic_string))
					{
						prev_arg_was_variadic = true;

						if (flag_variadic)
							panic("Line %d: '%s' specified more than once.\n", line_number, argument_optional_string);

						flag_variadic = true;
					}
					else if (token_equal(curr, argument_id_string))
					{
						flag_id = true;
					}
					else
					{
						break;
					}
				}

				if (!flag_optional && prev_arg_was_optional)
					panic("Line %d: Cannot have non-optional argument after optional argument.\n", line_number);

				uint32_t name_idx = ~0u;

				for (uint32_t i = 0; i != _countof(argument_type_names); ++i)
					if (token_equal(curr, argument_type_names[i]))
					{
						name_idx = i;

						spird::arg_flags flags = spird::arg_flags::none;

						

						if (flag_optional)
							flags |= spird::arg_flags::optional;

						if (flag_variadic)
							flags |= spird::arg_flags::variadic;

						if (flag_id)
							flags |= spird::arg_flags::id;

						output.append(static_cast<spird::arg_type>(flags));

						output.append(static_cast<spird::arg_type>(i));

						break;
					}

				++curr_argc;

				if (name_idx == ~0u)
					parse_panic("argument-type", curr);

				state = pstate::args_name;
			}
			else
			{
				curr = skip_whitespace(curr + 1);

				state = pstate::elem_depends;
			}

			break;
		}
		case pstate::args_name:
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

				if (mode == spird::data_mode::all || mode == spird::data_mode::disassembly)
					output.append(curr, i);

				curr = skip_whitespace(curr + i + 1);
			}
			else
			{
				char c = '\0';

				if (mode == spird::data_mode::all || mode == spird::data_mode::disassembly)
					output.append(&c, 0);
			}

			state = pstate::args_type;

			break;
		}
		case pstate::elem_depends:
		{
			if (token_equal(curr, elem_depends_string))
			{
				if (*curr != ':')
					parse_panic(":", curr);

				curr = skip_whitespace(curr + 1);

				bool has_multiple_elems = false;

				if (*curr == '[')
				{
					has_multiple_elems = true;

					curr = skip_whitespace(curr + 1);
				}

				uint32_t dependency_count_idx = ~0u;

				if (!no_implies_and_depends)
					dependency_count_idx = output.reserve_byte();

				uint32_t dependency_count = 0;

				do {
					uint32_t capability_id = ~0u;

					for (uint32_t i = 0; i != _countof(capability_name_strings); ++i)
					{
						if (token_equal(curr, capability_name_strings[i]))
						{
							capability_id = capability_ids[i];

							break;
						}
					}

					if (capability_id == ~0u)
						parse_panic("capability-name", curr);

					if (!no_implies_and_depends)
						output.append(static_cast<uint16_t>(capability_id));

					++dependency_count;
				}
				while(has_multiple_elems && *curr != ']');

				if (has_multiple_elems)
					curr = skip_whitespace(curr + 1);

				if (dependency_count > 127)
					panic("More than 127 elements in 'depends' array (%d elements).\n", dependency_count);
				
				if (!no_implies_and_depends)
					output.overwrite(dependency_count_idx, dependency_count);

				state = pstate::elem_close;
			}
			else
			{
				state = pstate::elem_implies;
			}

			break;
		}
		case pstate::elem_implies:
		{
			if (token_equal(curr, elem_implies_string))
			{
				if (*curr != ':')
					parse_panic(":", curr);

				curr = skip_whitespace(curr + 1);

				bool has_multiple_elems = false;

				if (*curr == '[')
				{
					has_multiple_elems = true;

					curr = skip_whitespace(curr + 1);
				}

				uint32_t dependency_count_idx = ~0u;

				if (!no_implies_and_depends)
					dependency_count_idx = output.reserve_byte();

				uint32_t dependency_count = 0;

				do {
					uint32_t capability_id = ~0u;

					for (uint32_t i = 0; i != _countof(capability_name_strings); ++i)
					{
						if (token_equal(curr, capability_name_strings[i]))
						{
							capability_id = capability_ids[i];

							break;
						}
					}

					if (capability_id == ~0u)
						parse_panic("capability-name", curr);

					if (!no_implies_and_depends)
						output.append(static_cast<uint16_t>(capability_id));

					++dependency_count;
				}
				while(has_multiple_elems && *curr != ']');

				if (has_multiple_elems)
					curr = skip_whitespace(curr + 1);

				if (dependency_count > 127)
					panic("More than 127 elements in 'depends' array (%d elements).\n", dependency_count);

				if (!no_implies_and_depends)
					output.overwrite(dependency_count_idx, dependency_count | 0x80);
			}
			else
			{
				if (!no_implies_and_depends)
				{
					uint32_t idx = output.reserve_byte();

					output.overwrite(idx, 0);
				}
			}

			state = pstate::elem_close;

			break;
		}
		case pstate::elem_close:
		{
			if (*curr != '}')
				parse_panic("}", curr);

			output.overwrite(curr_index.byte_offset, curr_argc);

			instruction_indices[instruction_index_count++] = curr_index;

			curr = skip_whitespace(curr + 1);

			state = pstate::elem_open;

			break;
		}
		case pstate::enum_close:
		{
			if (instruction_index_count != 0)
			{
				create_hashtable(&table_headers[curr_enum_type].size, &hashtables[curr_enum_type]);

				enum_data[curr_enum_type] = output.steal(&enum_data_sizes[curr_enum_type]);

				instruction_index_count = 0;
			}

			state = pstate::enum_open;

			break;
		}
		}
	}

	FILE* output_file;

	if (fopen_s(&output_file, argv[2], "wb") != 0)
		panic("Could not open file %s for writing.\n", argv[2]);

	uint32_t enum_count = 0;

	for (uint32_t i = _countof(table_headers); i != 0; --i)
		if (table_headers[i - 1].size != 0)
		{
			enum_count = i;

			break;
		}

	spird::file_header file_header;
	file_header.version = 10 + static_cast<uint32_t>(mode) + (no_implies_and_depends ? 0 : 3);
	file_header.table_count = enum_count;

	if (fwrite(&file_header, 1, sizeof(file_header), output_file) != sizeof(file_header))
		panic("Could not write to file %s.\n", argv[2]);

	uint32_t table_offset = sizeof(file_header) + sizeof(table_headers[0]) * enum_count;

	for (uint32_t i = 0; i != enum_count; ++i)
	{
		if (table_headers[i].size == 0)
			continue;

		if (is_bitmask_enum[i >> 6] & (1ui64 << (i & 63)))
			table_headers[i].flags |= spird::enum_flags::bitmask;

		table_headers[i].offset = table_offset;

		table_offset += table_headers[i].size * sizeof(spird::elem_index);
	}

	if (fwrite(table_headers, 1, enum_count * sizeof(table_headers[0]), output_file) != enum_count * sizeof(table_headers[0]))
		panic("Could not write to file %s.\n", argv[2]);

	uint32_t enum_offset = table_offset;

	for (uint32_t i = 0; i != enum_count; ++i)
	{
		if (hashtables[i] == nullptr)
			continue;

		for (uint32_t j = 0; j != table_headers[i].size; ++j)
			if (hashtables[i][j].id != ~0u)
				hashtables[i][j].byte_offset += enum_offset;

		enum_offset += enum_data_sizes[i];

		if (fwrite(hashtables[i], 1, table_headers[i].size * sizeof(spird::elem_index), output_file) != table_headers[i].size * sizeof(spird::elem_index))
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
