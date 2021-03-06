#ifndef SPV_DATA_BUILDER_STRINGS_HPP_INCLUDE_GUARD
#define SPV_DATA_BUILDER_STRINGS_HPP_INCLUDE_GUARD

#include <cstdint>

static constexpr const char* const extended_help_string =
R"(
Utility for assembling SPIR-V instruction information from JSON into a format that "

inputfile:

File that is used as input for building SPIR-V instruction data.
Expected format: Pseudo-JSON with the following structure

"instructions": [
	{
		opcode: instruction-opcode
		name: "instruction-name"
		args: [
				[OPT] [VAR] INSN_TYPE [: "argument-name"]
				...
		]
	}
	...
]

Note that lines starting in '#' are comments.

The "instructions" array can be in any order, independent of instruction names
and opcodes. "args" must however reflect the order in which the arguments
appear in the instruction's binary form, meaning that the first SPIR-V argument
word must also be the first entry in the "args" array, and so on. 

INSN_TYPE must be one of
	ID           -> 0
	STR          -> 1
	U32          -> 2
	I32          -> 3
	F32          -> 4
	RST          -> 5
	RTYPE        -> 6
	DECO         -> 7
	LANGID       -> 8
	TYPID        -> 9
	MEMBER       -> 10

and indicates argument's type.
NOTE: RESULT also represents an ID type, but carries the additional semantic
information, that the argument is the instruction's Result <id>.

The optional flags OPT and VAR indicate that the presence of the argument is
optional or that the argument is in fact variable-length list of arguments.
If the variable list may be zero arguments long, OPT and ARG must both be
specified. Otherwise, it is assumed that the list has at least one element.


outputfile:

File that receives the built SPIR-V instruction data.
The format (in pseudo-c) is:

struct argument
{
	uint8_t spirv_insn_argtype;
	char argument_name[];
};

struct instruction
{
	uint8_t argument_count;
	char instruction_name[];
	argument arguments[
};

struct instruction_index
{
	uint32_t opcode;
	uint32_t instruction_offset;
}

uint32_t instruction_hashtable_size;

instruction_index instruction_hashtable[];

instruction instructions[];

where instruction_hashtable_size is the number of uint32_t in
instruction_hashtable, and instruction_hashtable holds the byte-offset into the
instructions array at which the given instruction is located.
All character strings are null terminated, meaning that a '\\0' indicates the
beginning of the next field.
)";


static constexpr const char* const basic_help_string =
R"(
inputfile: Name of the file that contains the textual input which is used to build SPIR-V instruction
data.
outputfile: Name of the file that receives the built SPIR-V instruction data.
See --help for further information.
)";

static constexpr const char* const elem_id_string           = "id";

static constexpr const char* const elem_name_string         = "name";

static constexpr const char* const elem_args_string         = "args";

static constexpr const char* const elem_depends_string      = "depends";

static constexpr const char* const elem_implies_string      = "implies";

static constexpr const char* const argument_optional_string = "OPT";

static constexpr const char* const argument_variadic_string = "VAR";

static constexpr const char* const argument_id_string       = "ID";

static constexpr const char* const argument_result_string   = "RST";

static constexpr const char* const argument_pair_string     = "PAIR";

static constexpr const char* const argument_const_string    = "CONST";



static constexpr const char enum_flag_char = '@';

static constexpr const char* const enum_flag_strings[]{
	"BITMASK",
	"NAMED",
};

#endif // SPV_DATA_BUILDER_STRINGS_HPP_INCLUDE_GUARD
