#include "spird_names.hpp"

#include <cstring>

static constexpr const char* const arg_type_names_low_part[]
{
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
	"MEMORYSEMANTICS",
	"MEMORYOPERANDS",
	"SCOPE",
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
};

static constexpr const char* const arg_type_names_high_part[]
{
	"NAMEDENUM",
	"Auto",
	"Void",
	"Bool",
	"Int",
	"Float",
	"Vector",
	"Matrix",
	"Image",
	"Sampler",
	"SampledImage",
	"Array",
	"RuntimeArray",
	"Struct",
	"Opaque",
	"Pointer",
	"Function",
	"Event",
	"DeviceEvent",
	"ReserveId",
	"Queue",
	"Pipe",
	"PipeStorage",
	"NamedBarrier",
	"BufferSurfaceINTEL",
	"RayQueryKHR",
	"AccelerationStructureKHR",
	"CooperativeMatrixNV",
	"String",
	"ExtInstSet",
	"Label",
	"DecoGroup",
	"U32",
	"I64",
	"MEMBER",
	"VALUE",
	"TYPE",
	"ARG",
	"LITERAL",
	"UNKNOWN",
	"RTYPE",
	"RST",
};

static constexpr const char* const capability_names[]
{
	"Matrix",
	"Shader",
	"Geometry",
	"Tesselation",
	"Addresses",
	"Linkage",
	"Kernel",
	"Vector16",
	"Float16Buffer",
	"Float16",
	"Float64",
	"Int64",
	"Int64Atomics",
	"ImageBasic",
	"ImageReadWrite",
	"ImageMipmap",
	"Pipes",
	"Groups",
	"DeviceEnqueue",
	"LiteralSampler",
	"AtomicStorage",
	"Int16",
	"TesselationPointSize",
	"GeometryPointSize",
	"ImageGatherExtended",
	"StorageImageMultisample",
	"UniformBufferArrayDynamicIndexing",
	"SampledImageArrayDynamicIndexing",
	"StorageBufferArrayDynamicIndexing",
	"StorageImageArrayDynamicIndexing",
	"ClipDistance",
	"CullDistance",
	"ImageCubeArray",
	"SampleRateShading",
	"ImageRect",
	"SampledRect",
	"GenericPointer",
	"Int8",
	"InputAttachment",
	"SparseResidency",
	"MinLod",
	"Sampled1D",
	"Image1D",
	"SampledCubeArray",
	"SampledBuffer",
	"ImageBuffer",
	"ImageMSArray",
	"StorageImageExtendedFormats",
	"ImageQuery",
	"DerivativeControl",
	"InterpolationFunction",
	"TransformFeedback",
	"GeometryStreams",
	"StorageImageReadWithoutFormat",
	"StorageImageWriteWithoutFormat",
	"MultiViewport",
	"SubgroupDispatch",
	"NamedBarrier",
	"PipeStorage",
	"GroupNonUniform",
	"GroupNonUniformVote",
	"GroupNonUniformArithmetic",
	"GroupNonUniformBallot",
	"GroupNonUniformShuffle",
	"GroupNonUniformShuffleRelative",
	"GroupNonUniformClustered",
	"GroupNonUniformQuad",
	"ShaderLayer",
	"ShaderViewportIndex",
	"UniformDecoration",
	"FragmentShadingRateKHR",
	"SubgroupBallotKHR",
	"DrawParameters",
	"WorkgroupMemoryExplicitLayoutKHR",
	"WorkgroupMemoryExplicitLayout8BitAccessKHR",
	"WorkgroupMemoryExplicitLayout16BitAccessKHR",
	"SubgroupVoteKHR",
	"StorageBuffer16BitAccess",
	"UniformAndStorageBuffer16BitAcess",
	"StoragePushConstant16",
	"StorageInputOutput16",
	"DeviceGroup",
	"MultiView",
	"VariablePointersStorageBuffer",
	"VariablePointers",
	"AtomicStorageOps",
	"SampleMaskPostDepthCoverage",
	"StorageBuffer8BitAcess",
	"UniformAndStorageBuffer8BitAccess",
	"StoragePushConstant8",
	"DenormPreserve",
	"DenormFlushToZero",
	"SignedZeroInfNanPreserve",
	"RoundingModeRTE",
	"RoundingModeRTZ",
	"RayQueryProvisionalKHR",
	"RayQueryKHR",
	"RayTraversalPrimitiveCullingKHR",
	"RayTracingKHR",
	"Float16ImageAMD",
	"ImageGatherBiasLodAMD",
	"FragmentMaskAMD",
	"StencilExportEXT",
	"ImageReadWriteLodAMD",
	"Int64ImageEXT",
	"ShaderClockKHR",
	"SampleMaskOverrideCoverageNV",
	"GeometryShaderPassthroughNV",
	"ShaderViewportIndexLayerEXT",
	"ShaderViewportMaskNV",
	"ShaderStereoViewNV",
	"PerViewAttributesNV",
	"FragmentFullyCoveredEXT",
	"MeshShadingNV",
	"ImageFootprintNV",
	"FragmentBarycentricKHR",
	"ComputeDerivativeGroupQuadsNV",
	"FragmentDensityEXT",
	"GroupNonUniformPartitionedNV",
	"ShaderNonUniform",
	"RuntimeDescriptorArray",
	"InputAttachmentArrayDynamicIndexing",
	"UniformTexelBufferArrayDynamicIndexing",
	"StorageTexelBufferArrayDynamicIndexing",
	"UniformBufferArrayNonUniformIndexing",
	"SampledImageArrayNonUniformIndexing",
	"StorageBufferArrayNonUniformIndexing",
	"StorageImageArrayNonUniformIndexing",
	"InputAttachmentArrayNonUniformIndexing",
	"UniformTexelBufferArrayNonUniformIndexing",
	"StorageTexelBufferArrayNonUniformIndexing",
	"RayTracingNV",
	"RayTracingMotionBlurNV",
	"VulkanMemoryModel",
	"VulkanMemoryModelDeviceScope",
	"PhysicalStorageBufferAddresses",
	"ComputeDerivativeGroupLinearNV",
	"RayTracingProvisionalKHR",
	"CooperativeMatrixNV",
	"FragmentShaderSampleInterlockEXT",
	"FragmentShaderShadingRateInterlockEXT",
	"ShaderSMBuiltinsNV",
	"FragmentShaderPixelInterlockEXT",
	"DemoteToHelperInvocation",
	"BindlessTextureNV",
	"SubgroupShuffleINTEL",
	"SubgroupBufferBlockIOINTEL",
	"SubgroupImageBlockIOINTEL",
	"SubgroupImageMediaBlockIOINTEL",
	"RoundToInfinityINTEL",
	"FloatingPointModeINTEL",
	"IntegerFunctions2INTEL",
	"FunctionPointersINTEL",
	"IndirectReferencesINTEL",
	"AsmINTEL",
	"AtomicFloat32MinMaxEXT",
	"AtomicFloat64MinMaxEXT",
	"AtomicFloat16MinMaxEXT",
	"VectorComputeINTEL",
	"VectorAnyINTEL",
	"ExpectAssumeKHR",
	"SubgroupAvcMotionEstimationINTEL",
	"SubgroupAvcMotionEstimationIntraINTEL",
	"SubgroupAvcMotionEstimationChromaINTEL",
	"VariableLengthArrayINTEL",
	"FunctionFloatControlINTEL",
	"FPGAMemoryAttributesINTEL",
	"FPFastMathModeINTEL",
	"ArbitraryPrecisionIntegersINTEL",
	"ArbitraryPrecisionFloatingPointINTEL",
	"UnstructuredLoopControlsINTEL",
	"FPGALoopControlsINTEL",
	"KernelAttributesINTEL",
	"FPGAKernelAttributesINTEL",
	"FPGAMemoryAccessesINTEL",
	"FPGAClusterAttributesINTEL",
	"LoopFuseINTEL",
	"FPGABufferLocationINTEL",
	"ArbitraryPrecisionFixedPointINTEL",
	"USMStorageClassesINTEL",
	"IOPipesINTEL",
	"BlockingPipesINTEL",
	"FPGARegINTEL",
	"DotProductInputAll",
	"DotProductInput4x8Bit",
	"DotProductInput4x8BitPacked",
	"DotProduct",
	"BitInstructions",
	"AtomicFloat32AddEXT",
	"AtomicFloat64AddEXT",
	"LongConstantCompositeINTEL",
	"OptNoneINTEL",
	"AtomicFloat16AddEXT",
	"DebugInfoModuleINTEL",
};

static constexpr uint16_t capability_ids[]
{
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	4422,
	4423,
	4427,
	4428,
	4429,
	4430,
	4431,
	4433,
	4434,
	4435,
	4436,
	4437,
	4439,
	4441,
	4442,
	4445,
	4447,
	4448,
	4449,
	4450,
	4464,
	4465,
	4466,
	4467,
	4468,
	4471,
	4472,
	4478,
	4479,
	5008,
	5009,
	5010,
	5013,
	5015,
	5016,
	5055,
	5249,
	5251,
	5254,
	5255,
	5259,
	5260,
	5265,
	5266,
	5282,
	5284,
	5288,
	5291,
	5297,
	5301,
	5302,
	5303,
	5304,
	5305,
	5306,
	5307,
	5308,
	5309,
	5310,
	5311,
	5312,
	5340,
	5341,
	5345,
	5346,
	5347,
	5350,
	5353,
	5357,
	5363,
	5372,
	5373,
	5378,
	5379,
	5390,
	5568,
	5569,
	5570,
	5579,
	5582,
	5583,
	5584,
	5603,
	5604,
	5606,
	5612,
	5613,
	5616,
	5617,
	5619,
	5629,
	5696,
	5697,
	5698,
	5817,
	5821,
	5824,
	5837,
	5844,
	5845,
	5886,
	5888,
	5892,
	5897,
	5898,
	5904,
	5906,
	5920,
	5922,
	5935,
	5943,
	5945,
	5948,
	6016,
	6017,
	6018,
	6019,
	6025,
	6033,
	6034,
	6089,
	6094,
	6095,
	6114,
};

static constexpr const char* const enum_names[]
{
	"Instruction",
	"SourceLanguage",
	"ExecutionModel",
	"AddressingModel",
	"MemoryModel",
	"ExecutionMode",
	"StorageClass",
	"Dim",
	"SamplerAddressingMode",
	"SamplerFilterMode",
	"ImageFormat",
	"ImageChannelOrder",
	"ImageChannelDataType",
	"ImageOperands",
	"FpFastMathMode",
	"FpRoundingMode",
	"LinkageType",
	"AccessQualifier",
	"FunctionParameterAttribute",
	"Decoration",
	"Builtin",
	"SelectionControl",
	"LoopControl",
	"FunctionControl",
	"MemorySemantics",
	"MemoryOperands",
	"Scope",
	"GroupOperation",
	"KernelEnqueueFlags",
	"KernelProfilingInfo",
	"Capability",
	"ReservedRayFlags",
	"ReservedRayQueryIntersection",
	"ReservedRayQueryCommittedType",
	"ReservedRayQueryCandidateType",
	"ReservedFragmentShadingRate",
	"ReservedFpDenormMode",
	"ReservedFpOperationMode",
	"QuantizationMode",
	"OverflowMode",
	"PackedVectorFormat",
};

static bool names_equal(const char* a, uint32_t a_bytes, const char* b) noexcept
{
	if (a_bytes == 0)
	{
		if (strlen(b) == strlen(a) && strcmp(b, a) == 0)
			return true;
	}
	else
	{
		if (strlen(b) == a_bytes && strncmp(b, a, a_bytes) == 0)
			return true;
	}

	return false;
}

bool spird::get_name_from_arg_type(spird::arg_type type, const char** out_name) noexcept
{
	if (static_cast<uint8_t>(type) < sizeof(arg_type_names_low_part) / sizeof(*arg_type_names_low_part))
	{
		*out_name = arg_type_names_low_part[static_cast<uint8_t>(type)];

		return true;
	}

	if (static_cast<uint8_t>(type) >= 256 - sizeof(arg_type_names_high_part) / sizeof(*arg_type_names_high_part))
	{
		*out_name = arg_type_names_high_part[static_cast<uint8_t>(type) - 256 + sizeof(arg_type_names_high_part) / sizeof(*arg_type_names_high_part)];

		return true;
	}

	return false;
}

bool spird::get_arg_type_from_name(const char* name, uint32_t bytes, spird::arg_type* out_type) noexcept
{
	for (uint32_t i = 0; i != sizeof(arg_type_names_low_part) / sizeof(*arg_type_names_low_part); ++i)
		if (names_equal(name, bytes, arg_type_names_low_part[i]))
		{
			*out_type = static_cast<spird::arg_type>(i);

			return true;
		}

	for (uint32_t i = 0; i != sizeof(arg_type_names_high_part) / sizeof(*arg_type_names_high_part); ++i)
		if (names_equal(name, bytes, arg_type_names_high_part[i]))
		{
			*out_type = static_cast<spird::arg_type>(256 - sizeof(arg_type_names_high_part) / sizeof(*arg_type_names_high_part) + i);

			return true;
		}

	return false;
}

bool spird::get_name_from_capability_id(uint16_t id, const char** out_name) noexcept
{
	for (uint32_t i = 0; i != sizeof(capability_ids) / sizeof(*capability_ids); ++i)
		if (capability_ids[i] == id)
		{
			*out_name = capability_names[i];

			return true;
		}

	return false;
}

bool spird::get_capability_id_from_name(const char* name, uint32_t bytes, uint16_t* out_id) noexcept
{
	for (uint32_t i = 0; i != sizeof(capability_names) / sizeof(*capability_names); ++i)
		if (names_equal(name, bytes, capability_names[i]))
		{
			*out_id = capability_ids[i];

			return true;
		}

	return false;
}

bool spird::get_name_from_enum_id(spird::enum_id id, const char** out_name) noexcept
{
	if (static_cast<uint32_t>(id) < sizeof(enum_names) / sizeof(*enum_names))
	{
		*out_name = enum_names[static_cast<uint32_t>(id)];

		return true;
	}

	return false;
}

bool spird::get_enum_id_from_name(const char* name, uint32_t bytes, spird::enum_id* out_id) noexcept
{
	for (uint32_t i = 0; i != sizeof(enum_names) / sizeof(*enum_names); ++i)
		if (names_equal(name, bytes, enum_names[i]))
		{
			*out_id = static_cast<spird::enum_id>(i);

			return true;
		}

	return false;
}
