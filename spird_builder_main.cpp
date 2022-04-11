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

	void append_u8(uint8_t v) noexcept
	{
		grow(1);

		m_data[m_used++] = v;
	}

	void append_u16(uint16_t v) noexcept
	{
		grow(2);

		m_data[m_used++] = static_cast<uint8_t>(v);

		m_data[m_used++] = static_cast<uint8_t>(v >> 8);
	}

	void append_str(const char* str, uint8_t bytes) noexcept
	{
		grow(bytes + 1);

		memcpy(m_data + m_used, str, bytes);

		m_used += bytes + 1;

		m_data[m_used - 1] = '\0';
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

struct arg_data
{
	spird::arg_flags flags = spird::arg_flags::none;
	spird::arg_type type = spird::arg_type::INSTRUCTION;
};

struct arg_state
{
	spird::arg_flags prev_flags;
	spird::arg_type prev_type;
	bool has_rtype;
};

struct elem_info
{
	uint32_t id;

	const char* name;
	
	uint8_t name_bytes;

	uint8_t argc;

	spird::arg_flags arg_flags[32];

	spird::arg_type arg_types[32];

	const char* arg_names[32];

	uint8_t arg_name_bytes[32];

	uint32_t implies_or_depends_count;

	uint8_t implies_or_depends[127];
};

struct enum_info
{
	spird::enum_flags flags;

	uint16_t hashtable_entries;

	uint32_t data_bytes;

	spird::elem_index* hashtable;
	
	const void* data;
};



static spird::elem_index s_data_indices[65536];

static enum_info s_enum_infos[spird::enum_id_count];

static output_data s_output;

uint32_t s_line_number = 1;



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
	panic("Line %d:Expected '%s'. Found '%s' instead.\n", s_line_number, expected, isolate_token_for_panic(instead));
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
			++s_line_number;

		if (*str == '\r')
		{
			++s_line_number;

			if (str[1] == '\n')
				++str;
		}

		++str;
	}

	return str;

}

static void create_hashtable(uint16_t elem_count, const spird::elem_index* elems, uint16_t* out_table_size, spird::elem_index** out_table) noexcept
{
	uint32_t table_size = (elem_count * 3) >> 1;

	if (table_size > 0xFFFF)
		panic("Size of table is greater than maximum of 0xFFFF.\n");

	spird::elem_index* table = static_cast<spird::elem_index*>(malloc(table_size * sizeof(spird::elem_index)));

	if (table == nullptr)
		panic("malloc failed.\n");

	memset(table, 0xFF, table_size * sizeof(spird::elem_index));

	uint16_t* offsets = static_cast<uint16_t*>(malloc(table_size * sizeof(uint16_t)));

	if (offsets == nullptr)
		panic("malloc failed.\n");

	memset(offsets, 0x00, table_size * sizeof(uint16_t));

	for (uint32_t i = 0; i != elem_count; ++i)
	{
		spird::elem_index elem = elems[i];

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

static bool token_equal_no_advance(const char* curr, const char* token) noexcept
{
	uint32_t len = 0;

	while (!is_whitespace(curr[len]) && curr[len] != '\0' && curr[len] != ':' && curr[len] != enum_flag_char)
		++len;

	if (token[len] != '\0')
		return false;

	if (strncmp(curr, token, len) != 0)
		return false;

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

static arg_data parse_arg_type(const char*& curr, arg_state* state) noexcept
{
	if ((state->prev_flags & (spird::arg_flags::variadic | spird::arg_flags::pair)) == spird::arg_flags::variadic)
		panic("Line %d: Cannot have another argument after variadic argument.\n", s_line_number - 1);

	uint32_t name_len;

	arg_data data;

	while (true)
	{
		name_len = 0;

		while (!is_whitespace(curr[name_len]))
			++name_len;

		if (token_equal(curr, argument_optional_string))
		{
			if ((data.flags & spird::arg_flags::optional) == spird::arg_flags::optional)
				panic("Line %d: '%s' specified more than once.\n", s_line_number, argument_optional_string);

			data.flags |= spird::arg_flags::optional;
		}
		else if (token_equal(curr, argument_variadic_string))
		{
			if ((data.flags & spird::arg_flags::variadic) == spird::arg_flags::variadic)
				panic("Line %d: '%s' specified more than once.\n", s_line_number, argument_variadic_string);

			data.flags |= spird::arg_flags::variadic;
		}
		else if (token_equal(curr, argument_id_string))
		{
			if ((data.flags & spird::arg_flags::result) == spird::arg_flags::result)
				panic("Line %d: Cannot combine RST and ID (RST implies ID).\n", s_line_number);

			if ((data.flags & spird::arg_flags::id) == spird::arg_flags::id)
				panic("Line %d: '%s' specified more than once.\n", s_line_number, argument_id_string);

			data.flags |= spird::arg_flags::id;
		}
		else if (token_equal(curr, argument_result_string))
		{
			if ((data.flags & spird::arg_flags::result) == spird::arg_flags::result)
				panic("Line %d: '%s' specified more than once.\n", s_line_number, argument_result_string);

			if ((data.flags & spird::arg_flags::id) == spird::arg_flags::id)
				panic("Line %d: Cannot combine RST and ID (RST implies ID).\n", s_line_number);

			data.flags |= spird::arg_flags::result | spird::arg_flags::id;
		}
		else if (token_equal(curr, argument_pair_string))
		{
			if ((data.flags & spird::arg_flags::pair) == spird::arg_flags::pair)
				panic("Line %d: '%s' specified more than once.\n", s_line_number, argument_pair_string);

			data.flags |= spird::arg_flags::pair;
		}
		else
		{
			break;
		}
	}

	if ((data.flags & spird::arg_flags::result) == spird::arg_flags::result && (data.flags & (spird::arg_flags::optional | spird::arg_flags::variadic)) != spird::arg_flags::none)
		panic("Line %d: Cannot combine RST with other flags.\n", s_line_number);

	if ((data.flags & spird::arg_flags::optional) != spird::arg_flags::optional &&
		(state->prev_flags & (spird::arg_flags::optional | spird::arg_flags::pair)) == spird::arg_flags::optional)
		panic("Line %d: Cannot have non-optional argument after optional argument.\n", s_line_number);

	if (token_equal_no_advance(curr, argument_type_names[static_cast<uint32_t>(spird::arg_type::RTYPE)]))
	{
		if (state->has_rtype)
			panic("Line %d: Cannot have more than one argument of type RTYPE.\n", s_line_number);

		state->has_rtype = true;

		data.flags |= spird::arg_flags::id;
	}

	if ((data.flags & spird::arg_flags::result) == spird::arg_flags::none)
	{
		bool found = false;

		for (uint32_t i = 0; i != _countof(argument_type_names); ++i)
			if (token_equal(curr, argument_type_names[i]))
			{
				found = true;

				data.type = static_cast<spird::arg_type>(i);

				break;
			}
			
		if (!found)
			parse_panic("argument-type", curr);
	}
	else
	{
		bool found = false;

		for (uint32_t i = 0; i != _countof(result_type_names); ++i)
			if (token_equal(curr, result_type_names[i]))
			{
				found = true;

				if (static_cast<spird::rst_type>(i) == spird::rst_type::Auto && !state->has_rtype)
					panic("Line %d: Cannot have result of type %s without RTYPE.\n", s_line_number, result_type_names[static_cast<uint32_t>(spird::rst_type::Auto)]);

				data.type = static_cast<spird::arg_type>(i);

				break;
			}

		if (!found)
			parse_panic("result-type", curr);
	}

	state->prev_flags = data.flags;

	state->prev_type = data.type;

	return data;
}

void parse_elem(const char*& curr, elem_info* out_info) noexcept
{
	if (!token_equal(curr, "{"))
		parse_panic("{", curr);

	if (!token_equal(curr, elem_id_string))
		parse_panic(elem_id_string, curr);

	if (*curr != ':')
		parse_panic(":", curr);

	curr = skip_whitespace(curr + 1);

	uint32_t id = 0;

	if (*curr == '0' && curr[1] == 'x' || curr[1] == 'X')
	{
		curr += 2;

		if ((*curr < '0' || *curr > '9') && (*curr < 'a' || *curr > 'f') && (*curr < 'A' || *curr > 'F'))
			parse_panic("0x[0-9a-fA-F]+", curr - 2);

		while(true)
		{
			if (*curr >= '0' && *curr <= '9')
				id = id * 16 + *curr++ - '0';
			else if (*curr >= 'a' && *curr <= 'f')
				id = id * 16 + *curr++ - 'a' + 10;
			else if (*curr >= 'A' && *curr <= 'F')
				id = id * 16 + *curr++ - 'A' + 10;
			else
				break;
		}
	}
	else
	{
		if (*curr < '0' || *curr > '9')
			parse_panic("[0-9]+", curr);

		while(*curr >= '0' && *curr <= '9')
			id = id * 10 + *curr++ - '0';
	}

	out_info->id = id;

	curr = skip_whitespace(curr);

	if (!token_equal(curr, elem_name_string))
		parse_panic(elem_name_string, curr);

	if (*curr != ':')
		parse_panic(":", curr);

	curr = skip_whitespace(curr + 1);

	if (*curr != '"')
		parse_panic("\"elem-name\"", curr);

	out_info->name = curr + 1;

	uint32_t name_bytes = 0;

	for (; curr[name_bytes + 1] != '"'; ++name_bytes)
	{
		if (curr[name_bytes + 1] == '\r' || curr[name_bytes + 1] == '\n' || curr[name_bytes + 1] == '\0')
			parse_panic("\"elem-name\"", curr);
	}

	if (name_bytes == 0)
		parse_panic("\"elem-name\"", curr);

	if (name_bytes > 255)
		panic("Line %d: Name exceeds maximum of 255 bytes (%d bytes).\n", s_line_number, name_bytes);

	out_info->name_bytes = name_bytes;

	curr = skip_whitespace(curr + name_bytes + 2);

	if (token_equal(curr, elem_args_string))
	{
		if (*curr != ':')
			parse_panic(":", curr);

		curr = skip_whitespace(curr + 1);

		if (!token_equal(curr, "["))
			parse_panic("[", curr);

		arg_state state{};

		uint32_t argc = 0;

		while(!token_equal(curr, "]"))
		{
			if (argc >= _countof(elem_info::arg_types))
				panic("Line %d: Element has more than %d arguments.\n", _countof(elem_info::arg_types));

			const bool is_pair_continued = (state.prev_flags & spird::arg_flags::pair) == spird::arg_flags::pair;

			arg_data arg = parse_arg_type(curr, &state);

			if (is_pair_continued)
			{
				if ((arg.flags & (spird::arg_flags::optional | spird::arg_flags::variadic | spird::arg_flags::result | spird::arg_flags::pair)) != spird::arg_flags::none)
					panic("Line %d: Second element of argument pair can only have ID, CONST or FORWARD flags set.\n", s_line_number);
			}

			out_info->arg_flags[argc] = arg.flags;

			out_info->arg_types[argc] = arg.type;

			if (*curr == '"')
			{
				out_info->arg_names[argc] = curr + 1;

				uint32_t arg_name_bytes = 0;

				for (; curr[arg_name_bytes + 1] != '"'; ++arg_name_bytes)
				{
					if (curr[arg_name_bytes + 1] == '\r' || curr[arg_name_bytes + 1] == '\n' || curr[arg_name_bytes + 1] == '\0')
						parse_panic("\"arg-name\"", curr);
				}

				if (arg_name_bytes == 0)
					parse_panic("\"arg-name\"", curr);

				if (name_bytes > 255)
					panic("Line %d: Name exceeds maximum of 255 bytes (%d bytes).\n", s_line_number, name_bytes);

				out_info->arg_name_bytes[argc] = arg_name_bytes;

				curr = skip_whitespace(curr + arg_name_bytes + 2);
			}
			else
			{
				out_info->arg_names[argc] = ""; // Not nullptr for memcpy

				out_info->arg_name_bytes[argc] = 0;
			}

			++argc;
		}

		out_info->argc = argc;
	}
	else
	{
		out_info->argc = 0;
	}

	bool has_depends = false, has_implies = false;

	if (token_equal(curr, elem_depends_string))
	{
		has_depends = true;
	}
	else if (token_equal(curr, elem_implies_string))
	{
		has_implies = true;
	}

	if (has_depends || has_implies)
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

		uint32_t implies_or_depends_count = 0;

		do {
			if (implies_or_depends_count > 127)
				panic("Line %d: More than 127 elements in 'depends' array (%d elements).\n", s_line_number, implies_or_depends_count);

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

			out_info->implies_or_depends[implies_or_depends_count] = capability_id;

			++implies_or_depends_count;
		}
		while(has_multiple_elems && *curr != ']');

		if (has_multiple_elems)
			curr = skip_whitespace(curr + 1);

		out_info->implies_or_depends_count = implies_or_depends_count;
	}
	else
	{
		out_info->implies_or_depends_count = 0;
	}

	if (has_implies && out_info->implies_or_depends_count != 0)
	{
		out_info->implies_or_depends_count |= 0x80;
	}

	if (!token_equal(curr, "}"))
		parse_panic("}", curr);
}

void parse_enum(const char*& curr, spird::data_mode mode) noexcept
{
	uint32_t enum_id = ~0u;

	for (uint32_t i = 0; i != _countof(enum_name_strings); ++i)
		if(token_equal(curr, enum_name_strings[i]))
		{
			enum_id = i;

			break;
		}

	if (enum_id == ~0u)
		parse_panic("enum-name", curr);

	enum_info& out_info = s_enum_infos[enum_id];

	spird::enum_flags flags = spird::enum_flags::none;

	if (*curr == '@')
	{
		++curr;

		for (uint32_t i = 0; i != _countof(enum_flag_strings); ++i)
			if (token_equal(curr, enum_flag_strings[i]))
			{
				flags = static_cast<spird::enum_flags>(i + 1);				

				break;
			}

		if (flags == spird::enum_flags::none)
			parse_panic("enum-flag", curr);
	}

	out_info.flags = flags;

	out_info.data_bytes = 0;
	
	if (*curr != ':')
		parse_panic(":", curr);

	curr = skip_whitespace(curr + 1);

	if (!token_equal(curr, "["))
		parse_panic("[", curr);

	out_info.data = s_output.data() + s_output.size();

	uint32_t index_count = 0;

	while(*curr != ']')
	{
		if (index_count >= _countof(s_data_indices))
			panic("Line %d: Exceeded maximum of %d elements in enumeration %s.\n", s_line_number, _countof(s_data_indices), enum_name_strings[enum_id]);

		elem_info elem;

		parse_elem(curr, &elem);

		s_data_indices[index_count].id = elem.id;

		s_data_indices[index_count].byte_offset = s_output.size();

		index_count++;

		s_output.append_u8(elem.argc);

		s_output.append_str(elem.name, elem.name_bytes);

		for (uint8_t i = 0; i != elem.argc; ++i)
		{
			s_output.append_u8(static_cast<uint8_t>(elem.arg_flags[i]));

			s_output.append_u8(static_cast<uint8_t>(elem.arg_types[i]));

			s_output.append_str(elem.arg_names[i], elem.arg_name_bytes[i]);
		}

		s_output.append_u8(elem.implies_or_depends_count);

		for (uint8_t i = 0; i != (elem.implies_or_depends_count & 0x7F); ++i)
			s_output.append_u16(elem.implies_or_depends[i]);
	}

	curr = skip_whitespace(curr + 1);

	out_info.data_bytes = s_output.data() + s_output.size() - out_info.data;

	create_hashtable(index_count, s_data_indices, &out_info.hashtable_entries, &out_info.hashtable);
}

char* read_input(const char* input_filename) noexcept
{
	FILE* input_file;

	if (fopen_s(&input_file, input_filename, "rb") != 0)
		panic("Could not open file %s for reading.\n", input_filename);

	if (fseek(input_file, 0, SEEK_END) != 0)
		panic("Could not seek to end of input file %s.\n", input_file);

	size_t input_bytes = ftell(input_file);

	if (input_bytes < 0)
		panic("Could not determine size of input file %s.\n", input_file);

	if (fseek(input_file, 0, SEEK_SET) != 0)
		panic("Could not seek to start of input file %s.\n", input_file);

	char* input = static_cast<char*>(malloc(static_cast<size_t>(input_bytes + 1)));

	if (input == nullptr)
		panic("malloc failed\n");

	size_t actual_bytes_read = fread(input, 1, input_bytes, input_file);

	fclose(input_file);

	if (actual_bytes_read != input_bytes)
		panic("Failed to read from file %s.", input_filename);

	input[input_bytes] = '\0';

	return input;
}

void write_output(const char* output_filename) noexcept
{
	output_data& od = s_output;

	FILE* output_file;

	if (fopen_s(&output_file, output_filename, "wb") != 0)
		panic("Could not open file %s for writing.\n", output_filename);

	uint32_t enum_count = 0;

	for (uint32_t i = _countof(s_enum_infos); i != 0; --i)
		if (s_enum_infos[i - 1].hashtable_entries != 0)
		{
			enum_count = i;

			break;
		}

	spird::file_header file_header;
	file_header.version = 13;
	file_header.table_count = enum_count;

	if (fwrite(&file_header, 1, sizeof(file_header), output_file) != sizeof(file_header))
		panic("Could not write to file %s.\n", output_filename);

	uint32_t hashtable_offset = sizeof(spird::file_header) + sizeof(spird::table_header) * enum_count;

	spird::table_header table_headers[spird::enum_id_count];
	
	memset(table_headers, 0x00, sizeof(table_headers));

	for (uint32_t i = 0; i != enum_count; ++i)
	{
		if (s_enum_infos[i].hashtable_entries != 0)
		{
			table_headers[i].flags = s_enum_infos[i].flags;

			table_headers[i].offset = hashtable_offset;

			table_headers[i].size = s_enum_infos[i].hashtable_entries;

			hashtable_offset += table_headers[i].size * sizeof(spird::elem_index);
		}
	}

	if (fwrite(table_headers, 1, enum_count * sizeof(spird::table_header), output_file) != enum_count * sizeof(spird::table_header))
		panic("Could not write to file %s.\n", output_filename);

	uint32_t data_offset = hashtable_offset;

	for (uint32_t i = 0; i != enum_count; ++i)
	{
		if (s_enum_infos[i].hashtable_entries == 0)
			continue;

		for (uint32_t j = 0; j != s_enum_infos[i].hashtable_entries; ++j)
			if (s_enum_infos[i].hashtable[j].id != ~0u)
				s_enum_infos[i].hashtable[j].byte_offset += data_offset;

		if (fwrite(s_enum_infos[i].hashtable, 1, s_enum_infos[i].hashtable_entries * sizeof(spird::elem_index), output_file) != s_enum_infos[i].hashtable_entries * sizeof(spird::elem_index))
			panic("Could not write to file %s.\n", output_filename);
	}

	if (fwrite(s_output.data(), 1, s_output.size(), output_file) != s_output.size())
		panic("Could not write to file %s.\n", output_filename);

	fclose(output_file);
}

int main(int argc, const char** argv)
{
	prog_name = argv[0];

	const char* input_filename, * output_filename;

	spird::data_mode mode;

	bool no_implies_and_depends;

	if (!parse_args(argc, argv, &input_filename, &output_filename, &mode, &no_implies_and_depends))
		return 1;

	char* input_data = read_input(input_filename);

	const char* curr = skip_whitespace(input_data);

	while(*curr != '\0')
		parse_enum(curr, mode);

	write_output(output_filename);

	return 0;
}
