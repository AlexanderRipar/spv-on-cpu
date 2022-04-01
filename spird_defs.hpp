#ifndef SPV_DATA_DEFS_HPP_INCLUDE_GUARD
#define SPV_DATA_DEFS_HPP_INCLUDE_GUARD

#include <cstdint>

namespace spird
{
	enum class enum_flags : uint16_t;

	struct elem_index
	{
		uint32_t id;
		
		uint32_t byte_offset;
	};

	struct file_header
	{
		uint32_t version;

		uint32_t table_count;
	};

	struct table_header
	{
		uint16_t size;

		enum_flags flags;

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
		VALUE                         = 44,
		TYPE                          = 45,
		UNKNOWN                       = 46,
		U32                           = 47,
		STR                           = 48,
		ARG                           = 49,
		MEMBER                        = 50,
		U32IDPAIR                     = 51,
		IDMEMBERPAIR                  = 52,
		IDIDPAIR                      = 53,
		IDU32PAIR                     = 54,
		I64                           = 55,
	};

	enum class arg_flags : uint8_t
	{
		none     = 0x0,
		optional = 0x1,
		variadic = 0x2,
		id       = 0x4,
		result   = 0x8,
		constant = 0x10,
		forward  = 0x20,
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

	enum class enum_flags : uint16_t
	{
		none = 0x0,
		bitmask = 0x1
	};

	enum class data_mode : uint32_t
	{
		all,
		disassembly,
		debugging,
		all_no_implies_and_depends,
		disassembly_no_implies_and_depends,
		debugging_no_implies_and_depends,
	};

	enum class rst_type : uint8_t
	{
		Auto                     = 0,
		Void                     = 1,
		Bool                     = 2,
		Int                      = 3,
		Float                    = 4,
		Vector                   = 5,
		Matrix                   = 6,
		Image                    = 7,
		Sampler                  = 8,
		SampledImage             = 9,
		Array                    = 10,
		RuntimeArray             = 11,
		Struct                   = 12,
		Opaque                   = 13,
		Pointer                  = 14,
		Function                 = 15,
		Event                    = 16,
		DeviceEvent              = 17,
		ReserveId                = 18,
		Queue                    = 19,
		Pipe                     = 20,
		PipeStorage              = 21,
		NamedBarrier             = 22,
		BufferSurfaceINTEL       = 23,
		RayQueryKHR              = 24,
		AccelerationStructureKHR = 25,
		CooperativeMatrixNV      = 26,
		String                   = 27,
		ExtInstSet               = 28,
		Label                    = 29,
		DecoGroup                = 30,
	};

	inline enum_flags operator&(const enum_flags& lhs, const enum_flags& rhs) noexcept
	{
		return static_cast<enum_flags>(static_cast<uint16_t>(lhs) & static_cast<uint16_t>(rhs));
	}

	inline enum_flags operator|(const enum_flags& lhs, const enum_flags& rhs) noexcept
	{
		return static_cast<enum_flags>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
	}

	inline enum_flags& operator&=(enum_flags& lhs, const enum_flags& rhs) noexcept
	{
		lhs = lhs & rhs;
		
		return lhs;
	}

	inline enum_flags& operator|=(enum_flags& lhs, const enum_flags& rhs) noexcept
	{
		lhs = lhs | rhs;
		
		return lhs;
	}

	inline arg_flags operator&(const arg_flags& lhs, const arg_flags& rhs) noexcept
	{
		return static_cast<arg_flags>(static_cast<uint16_t>(lhs) & static_cast<uint16_t>(rhs));
	}

	inline arg_flags operator|(const arg_flags& lhs, const arg_flags& rhs) noexcept
	{
		return static_cast<arg_flags>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
	}

	inline arg_flags& operator&=(arg_flags& lhs, const arg_flags& rhs) noexcept
	{
		lhs = lhs & rhs;
		
		return lhs;
	}

	inline arg_flags& operator|=(arg_flags& lhs, const arg_flags& rhs) noexcept
	{
		lhs = lhs | rhs;
		
		return lhs;
	}

}

#endif // SPV_DATA_DEFS_HPP_INCLUDE_GUARD
