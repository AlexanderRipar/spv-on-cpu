#ifndef SPV_DATA_DEFS_HPP_INCLUDE_GUARD
#define SPV_DATA_DEFS_HPP_INCLUDE_GUARD

#include <cstdint>

namespace spird
{
	struct insn_index
	{
		uint32_t opcode;
		uint32_t byte_offset;
	};

	struct file_header
	{
		uint32_t version;
		uint32_t table_count;
	};

	struct table_header
	{
		uint32_t size;

		uint32_t offset;
	};
	
	static constexpr uint8_t insn_arg_optional_bit = 0x80;

	static constexpr uint8_t insn_arg_variadic_bit = 0x40;

	static constexpr uint8_t insn_argtype_mask     = 0x3F;
	
	static constexpr uint8_t enum_id_count = 41;

	enum class arg_type : uint8_t
	{
		INSTRUCTION                   = 0,
		SOURCELANGUAGE                = 1,
		EXECUTIONMODEL                = 2,
		ADDRESSINGMODEL               = 3,
		MEMORYMODEL                   = 4,
		EXECUTIONMODE                 = 5,
		STORAGECLASS                  = 6,
		DIM                           = 7,
		SAMPLERADDRESSINGMODE         = 8,
		SAMPLERFILTERMODE             = 9,
		IMAGEFORMAT                   = 10,
		IMAGECHANNELORDER             = 11,
		IMAGECHANNELDATATYPE          = 12,
		IMAGEOPERANDS                 = 13,
		FPFASTMATHMODE                = 14,
		FPROUNDINGMODE                = 15,
		LINKAGETYPE                   = 16,
		ACCESSQUALIFIER               = 17,
		FUNCTIONPARAMETERATTRIBUTE    = 18,
		DECORATION                    = 19,
		BUILTIN                       = 20,
		SELECTIONCONTROL              = 21,
		LOOPCONTROL                   = 22,
		FUNCTIONCONTROL               = 23,
		MEMORYSEMANTICSID             = 24,
		MEMORYOPERANDS                = 25,
		SCOPEID                       = 26,
		GROUPOPERATION                = 27,
		KERNELENQUEUEFLAGS            = 28,
		KERNELPROFILINGINFO           = 29,
		CAPABILITY                    = 30,
		RESERVEDRAYFLAGS              = 31,
		RESERVEDRAYQUERYINTERSECTION  = 32,
		RESERVEDRAYQUERYCOMMITTEDTYPE = 33,
		RESERVEDRAYQUERYCANDIDATETYPE = 34,
		RESERVEDFRAGMENTSHADINGRATE   = 35,
		RESERVEDFPDENORMMODE          = 36,
		RESERVEDFPOPERATIONMODE       = 37,
		QUANTIZATIONMODE              = 38,
		OVERFLOWMODE                  = 39,
		PACKEDVECTORFORMAT            = 40,

		RST                           = 41,
		RTYPE                         = 42,
		LITERAL                       = 43,
		ID                            = 44,
		TYPID                         = 45,
		U32                           = 46,
		STR                           = 47,
		ARG                           = 48,
		MEMBER                        = 49,
		U32IDPAIR                     = 50,
		IDMEMBERPAIR                  = 51,
		IDIDPAIR                      = 52,
		IDU32PAIR                     = 53,
		I64                           = 54,
		UNKNOWN                       = 255,
	};

	enum class enum_id : uint32_t
	{
		Instruction                   = 0,
		SourceLanguage                = 1,
		ExecutionModel                = 2,
		AddressingModel               = 3,
		MemoryModel                   = 4,
		ExecutionMode                 = 5,
		StorageClass                  = 6,
		Dim                           = 7,
		SamplerAddressingMode         = 8,
		SamplerFilterMode             = 9,
		ImageFormat                   = 10,
		ImageChannelOrder             = 11,
		ImageChannelDataType          = 12,
		ImageOperands                 = 13,
		FpFastMathMode                = 14,
		FpRoundingMode                = 15,
		LinkageType                   = 16,
		AccessQualifier               = 17,
		FunctionParameterAttribute    = 18,
		Decoration                    = 19,
		Builtin                       = 20,
		SelectionControl              = 21,
		LoopControl                   = 22,
		FunctionControl               = 23,
		MemorySemantics               = 24,
		MemoryOperands                = 25,
		Scope                         = 26,
		GroupOperation                = 27,
		KernelEnqueueFlags            = 28,
		KernelProfilingInfo           = 29,
		Capability                    = 30,
		ReservedRayFlags              = 31,
		ReservedRayQueryIntersection  = 32,
		ReservedRayQueryCommittedType = 33,
		ReservedRayQueryCandidateType = 34,
		ReservedFragmentShadingRate   = 35,
		ReservedFpDenormMode          = 36,
		ReservedFpOperationMode       = 37,
		QuantizationMode              = 38,
		OverflowMode                  = 39,
		PackedVectorFormat            = 40,
	};

	enum class data_mode : uint32_t
	{
		all,
		disassembly,
		debugging,
	};
}

#endif // SPV_DATA_DEFS_HPP_INCLUDE_GUARD
