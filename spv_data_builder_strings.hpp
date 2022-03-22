constexpr const char* const extended_help_string =
R"(
Utility for assembling SPIR-V instruction information from pseudo-JSON into a
format that can be consumed by other spv-on-cpu utilities.

inputfile:

File that is used as input for building SPIR-V instruction data.
Expected format: Pseudo-JSON with the following structure

	instructions: [
		{
			opcode: instruction-opcode
			name: "instruction-name"
			args: [
				{
					type: INSN_TYPE
					name: "argument-name"
				}
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
	STRING       -> 1
	U32          -> 2
	I32          -> 3
	F32          -> 4
	RESULT       -> 5
	RESULTTYPE   -> 6
	DECORATION   -> 7

and indicates argument's type.
NOTE: RESULT also represents an ID type, but carries the additional semantic
information, that the argument is the instruction's Result <id>.



outputfile:

File that receives the built SPIR-V instruction data.
The format (in pseudo-c) is:

	struct argument
	{
		uint8_t argument_type;
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
	};

	uint32_t version;

	uint32_t instruction_hashtable_size;

	instruction_index instruction_hashtable[];

	instruction instructions[];

where version indicates the type of hashing algorithm used for populating the
instruction_hashtable, and instruction_hashtable_size is the number of uint32_t
in instruction_hashtable, and instruction_hashtable holds the byte-offset into
the instructions array at which the given instruction is located.
All character strings are null terminated, meaning that a '\0' indicates the
beginning of the next field.
)";


constexpr const char* const basic_help_string =
R"(
inputfile: Name of the file that contains the JSON-input which is used to build SPIR-V instruction data."
outputfile: Name of the file that receives the built SPIR-V instruction data."
See --help for further information.
)";

constexpr const char* const instruction_array_string = "instructions";

constexpr const char* const instruction_name_string = "name";

constexpr const char* const instruction_opcode_string = "opcode";

constexpr const char* const instruction_args_string = "args";

constexpr const char* const argument_name_string = "name";

constexpr const char* const argument_type_string = "type";
