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



static constexpr const char* const enum_name_strings[]
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

static constexpr const char* const capability_name_strings[]
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

static constexpr const char* const data_type_strings[]
{
	"--for-all",
	"--for-disassembly",
	"--for-debugging",
};

static constexpr const char* const data_type_no_implies_and_depends_string = "--no-implies-and-depends";

static constexpr const char enum_flag_char = '@';

static constexpr const char* enum_flag_bitmask_string = "BITMASK";

#endif // SPV_DATA_BUILDER_STRINGS_HPP_INCLUDE_GUARD
