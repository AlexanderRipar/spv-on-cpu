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
	ID = 0,
	STR = 1,
	U32 = 2,
	I32 = 3,
	F32 = 4,
	RST = 5,
	RTYPE = 6,
	DECO = 7,
	LANGID = 8,
	TYPID = 9,
	MEMBER = 10,
	STRID = 11,
	DECOARG = 12,
	DECOARGID = 13,
	UNKNOWN = 14,
	ADDRMODEL = 15,
	MEMMODEL = 16,
	EXEMODEL = 17,
	EXEMODE = 18,
	CAPABILITY = 19,
	DIMENSION = 20,
	IMGFORMAT = 21,
	ACCESSQUAL = 22,
	STORAGECLASS = 23,
	SAMPADDRMODE = 24,
	SAMPFILTMODE = 25,
	OPCODE = 26,
	MEMOPERAND = 27,
	IMGOPERAND = 28,
	FUNCCTRL = 29,
	SELECTIONCTRL = 30,
	LOOPCTRL = 31,
	LOOPCTRLARG = 32,
	PACKEDVECFMT = 33,
	SCOPEID = 34,
	MEMSEMANTICID = 35,
	GROUPOP = 36,
};

static constexpr uint8_t spirv_insn_arg_optional_bit = 0x80;

static constexpr uint8_t spirv_insn_arg_variadic_bit = 0x40;

static constexpr uint8_t spirv_insn_argtype_mask = 0x3F;

#endif // SPV_DATA_DEFS_HPP_INCLUDE_GUARD
