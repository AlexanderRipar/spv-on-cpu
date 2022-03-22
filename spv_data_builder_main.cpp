#include "spv_data_builder_strings.hpp"

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

enum class argument_type : uint8_t
{
	ID = 0,
	STRING = 1,
	U32 = 2,
	I32 = 3,
	F32 = 4,
	RESULT = 5,
	RESULTTYPE = 6,
	DECORATION = 7,
};

static constexpr const char* argument_type_names[]{
	"ID",
	"STRING",
	"U32",
	"I32",
	"F32",
	"RESULT",
	"RESULTTYPE",
	"DECORATION",
};

enum class pstate
{
	seek_insn_open,
	seek_insn_name,
	seek_insn_opcode,
	seek_insn_args,
	seek_args_open,
	seek_args_name,
	seek_args_type,
	seek_args_close,
	seek_insn_close,
};

struct instruction_index
{
	uint32_t opcode;
	uint32_t byte_offset;
};

static uint32_t instruction_index_count = 0;

static instruction_index instruction_indices[65536];

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

	void append(const char* str, uint32_t len) noexcept
	{
		grow(len + 1);

		memcpy(m_data + m_used, str, len);

		m_used += len + 1;

		m_data[m_used - 1] = '\0';
	}

	void append(argument_type type) noexcept
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

		buf[i] = *str++;
	}

	return buf;
}

__declspec(noreturn) static void standard_panic(const char* expected, const char* instead) noexcept
{
	panic("Line %d:Expected %s. Found '%s' instead.\n", line_number, expected, isolate_token_for_panic(instead));
}

static const char* skip_whitespace(const char* str) noexcept
{
	while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' || *str == '#')
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

int main(int argc, const char** argv)
{
	prog_name = argv[0];

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s inputfile outputfile\n", argv[0]);

		if (argc == 2 && strcmp(argv[1], "--help") == 0)
			fputs(extended_help_string, stderr);
		else if (argc == 2 && strcmp(argv[1], "-h") == 0)
			fputs(basic_help_string, stderr);

		return 0;
	}

	FILE* output_file, * input_file;

	if (fopen_s(&input_file, argv[1], "rb"))
		panic("Could not open file %s for reading.\n", argv[1]);

	if (fopen_s(&output_file, argv[2], "wb"))
		panic("Could not open file %s for writing.\n", argv[2]);

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

	if (actual_bytes_read != input_bytes)
		panic("Failed to read from file %s.", argv[1]);

	input[input_bytes] = '\0';

	// Find start of instructions array
	const char* curr = skip_whitespace(input);

	if (strncmp(curr, instruction_array_string, strlen(instruction_array_string)) != 0)
		standard_panic(instruction_array_string, curr);

	curr = skip_whitespace(curr + strlen(instruction_array_string));

	if (*curr != ':')
		standard_panic(":", curr);

	curr = skip_whitespace(curr + 1);

	if (*curr != '[')
		standard_panic("[", curr);

	curr = skip_whitespace(curr + 1);

	pstate state = pstate::seek_insn_open;

	instruction_index curr_index;

	uint8_t curr_argc;

	output_data output;

	bool done = false;

	while (!done)
	{
		switch (state)
		{
		case pstate::seek_insn_open:
		{
			if (*curr == '{')
			{
				curr_argc = 0;

				curr_index.byte_offset = output.reserve_byte();

				state = pstate::seek_insn_opcode;

			}
			else if (*curr == ']')
			{
				done = true;
			}
			else
			{
				standard_panic("{|]", curr);
			}

			curr = skip_whitespace(curr + 1);

			break;
		}
		case pstate::seek_insn_opcode:
		{
			if (strncmp(curr, instruction_opcode_string, strlen(instruction_opcode_string)) != 0)
				standard_panic(instruction_opcode_string, curr);

			curr = skip_whitespace(curr + strlen(instruction_opcode_string));

			if (*curr != ':')
				standard_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			if (*curr < '0' || *curr > '9')
				standard_panic("[0-9]", curr);

			uint32_t opcode = 0;

			while (*curr >= '0' && *curr <= '9')
				opcode = opcode * 10 + *(curr++) - '0';

			if (opcode > 65535)
				panic("Line %d: Opcode %d exeeds the maximum of 65535.\n", line_number, opcode);

			curr_index.opcode = opcode;

			curr = skip_whitespace(curr);

			state = pstate::seek_insn_name;

			break;
		}
		case pstate::seek_insn_name:
		{
			if (strncmp(curr, instruction_name_string, strlen(instruction_name_string)) != 0)
				standard_panic(instruction_name_string, curr);

			curr = skip_whitespace(curr + strlen(instruction_name_string));

			if (*curr != ':')
				standard_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			if (*curr != '"')
				standard_panic("\"instruction-name\"", curr);

			++curr;

			uint32_t i = 0;

			while (curr[i] != '"')
			{
				if (curr[i] == '\0' || curr[i] == '\r' || curr[i] == '\n')
					standard_panic("\"instruction-name\"", curr);

				++i;
			}

			output.append(curr, i);

			curr = skip_whitespace(curr + i + 1);

			state = pstate::seek_insn_args;

			break;
		}
		case pstate::seek_insn_args:
		{
			if (strncmp(curr, instruction_args_string, strlen(instruction_args_string)) != 0)
				standard_panic(instruction_args_string, curr);

			curr = skip_whitespace(curr + strlen(instruction_args_string));

			if (*curr != ':')
				standard_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			if (*curr != '[')
				standard_panic("[", curr);

			curr = skip_whitespace(curr + 1);

			state = pstate::seek_args_open;

			break;
		}
		case pstate::seek_args_open:
		{
			if (*curr == '{')
			{
				if (curr_argc == 255)
					panic("Line %d: Number of instruction arguments exceeds maximum of 255.\n", line_number);

				++curr_argc;

				state = pstate::seek_args_type;
			}
			else if (*curr == ']')
			{
				state = pstate::seek_insn_close;
			}
			else
			{
				standard_panic("{|]", curr);
			}

			curr = skip_whitespace(curr + 1);

			break;
		}
		case pstate::seek_args_type:
		{
			if (strncmp(curr, argument_type_string, strlen(argument_type_string)) != 0)
				standard_panic(argument_type_string, curr);

			curr = skip_whitespace(curr + strlen(argument_type_string));

			if (*curr != ':')
				standard_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			uint32_t name_idx = ~0u;

			for (uint32_t i = 0; i != _countof(argument_type_names); ++i)
				if (strncmp(curr, argument_type_names[i], strlen(argument_type_names[i])) == 0)
				{
					name_idx = i;

					output.append(static_cast<argument_type>(i));
				}

			if (name_idx == ~0u)
				standard_panic("ID|STRING|U32|I32|F32|RESULT", curr);

			curr = skip_whitespace(curr + strlen(argument_type_names[name_idx]));

			state = pstate::seek_args_name;

			break;
		}
		case pstate::seek_args_name:
		{
			if (strncmp(curr, argument_name_string, strlen(argument_name_string)) != 0)
				standard_panic(argument_name_string, curr);

			curr = skip_whitespace(curr + strlen(argument_name_string));

			if (*curr != ':')
				standard_panic(":", curr);

			curr = skip_whitespace(curr + 1);

			if (*curr != '"')
				standard_panic("\"argument-name\"", curr);

			++curr;

			uint32_t i = 0;

			while (curr[i] != '"')
			{
				if (curr[i] == '\0' || curr[i] == '\r' || curr[i] == '\n')
					standard_panic("\"argument-name\"", curr);

				++i;
			}

			output.append(curr, i);

			curr = skip_whitespace(curr + i + 1);

			state = pstate::seek_args_close;

			break;
		}
		case pstate::seek_args_close:
		{
			if (*curr != '}')
				standard_panic("}", curr);

			curr = skip_whitespace(curr + 1);

			state = pstate::seek_args_open;

			break;
		}
		case pstate::seek_insn_close:
		{
			if (*curr != '}')
				standard_panic("}", curr);

			output.overwrite(curr_index.byte_offset, curr_argc);

			instruction_indices[instruction_index_count++] = curr_index;

			curr = skip_whitespace(curr + 1);

			state = pstate::seek_insn_open;

			break;
		}
		}
	}

	if (curr != input + input_bytes)
		standard_panic("End of file", curr);

	// TODO: Create Hashmap of indices

	// TODO: Write results

	return 0;
}
