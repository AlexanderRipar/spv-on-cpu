#ifndef SPV_DATA_DEFS_HPP_INCLUDE_GUARD
#define SPV_DATA_DEFS_HPP_INCLUDE_GUARD

#include <cstdint>

struct spirv_insn_index
{
	uint32_t opcode;
	uint32_t byte_offset;
};

struct spirv_data_header
{
	uint32_t version;
	uint32_t table_size;
};

enum class spirv_insn_argtype : uint8_t
{
	SOURCELANGUAGE                = 0,
	EXECUTIONMODEL                = 1,
	ADDRESSINGMODEL               = 2,
	MEMORYMODEL                   = 3,
	EXECUTIONMODE                 = 4,
	STORAGECLASS                  = 5,
	DIM                           = 6,
	SAMPLERADDRESSINGMODE         = 7,
	SAMPLERFILTERMODE             = 8,
	IMAGEFORMAT                   = 9,
	IMAGECHANNELORDER             = 10,
	IMAGECHANNELDATATYPE          = 11,
	IMAGEOPERANDS                 = 12,
	FPFASTMATHMODE                = 13,
	FPROUNDINGMODE                = 14,
	LINKAGETYPE                   = 15,
	ACCESSQUALIFIER               = 16,
	FUNCTIONPARAMETERATTRIBUTE    = 17,
	DECORATION                    = 18,
	BUILTIN                       = 19,
	SELECTIONCONTROL              = 20,
	LOOPCONTROL                   = 21,
	FUNCTIONCONTROL               = 22,
	MEMORYSEMANTICSID             = 23,
	MEMORYOPERANDS                = 24,
	SCOPEID                       = 25,
	GROUPOPERATION                = 26,
	KERNELENQUEUEFLAGS            = 27,
	KERNELPROFILINGINFO           = 28,
	CAPABILITY                    = 29,
	RESERVEDRAYFLAGS              = 30,
	RESERVEDRAYQUERYINTERSECTION  = 31,
	RESERVEDRAYQUERYCOMMITTEDTYPE = 32,
	RESERVEDRAYQUERYCANDIDATETYPE = 33,
	RESERVEDFRAGMENTSHADINGRATE   = 34,
	RESERVEDFPDENORMMODE          = 35,
	RESERVEDFPOPERATIONMODE       = 36,
	QUANTIZATIONMODE              = 37,
	OVERFLOWMODE                  = 38,
	PACKEDVECTORFORMAT            = 39,

	RST                           = 40,
	RTYPE                         = 41,
	LITERAL                       = 42,
	ID                            = 43,
	TYPID                         = 44,
	U32                           = 45,
	STR                           = 46,
	ARG                           = 47,
	MEMBER                        = 48,
	OPCODE                        = 49,
	LITERALIDPAIR                 = 50,
	
};

static constexpr uint8_t spirv_insn_arg_optional_bit = 0x80;

static constexpr uint8_t spirv_insn_arg_variadic_bit = 0x40;

static constexpr uint8_t spirv_insn_argtype_mask = 0x3F;

#endif // SPV_DATA_DEFS_HPP_INCLUDE_GUARD
