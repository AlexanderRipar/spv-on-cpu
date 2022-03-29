#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include "spird_accessor.hpp"

static constexpr const char* argument_type_names[]
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
	"MEMORYSEMANTICSID",
	"MEMORYOPERANDS",
	"SCOPEID",
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

	"RST",
	"RTYPE",
	"LITERAL",
	"ID",
	"TYPID",
	"U32",
	"STR",
	"ARG",
	"MEMBER",
	"U32IDPAIR",
	"IDMEMBERPAIR",
	"IDIDPAIR",
	"IDU32PAIR",
	"I64",
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

int main(int argc, const char** argv)
{
	if (argc != 2 && argc != 3)
	{
		printf("Usage: %s input-file [output-file]\n", argv[0]);
	}

	FILE* input_file;

	FILE* output_file = stdout;

	if (fopen_s(&input_file, argv[1], "rb") != 0)
	{
		printf("Could not open file %s for reading.\n", argv[1]);

		return 1;
	}

	if (fseek(input_file, 0, SEEK_END) != 0)
	{
		printf("Could not seek in file.\n");

		return 1;
	}

	size_t input_bytes = ftell(input_file);

	if (input_bytes < 0)
	{
		printf("Could not ftell file.\n");

		return 1;
	}

	if (fseek(input_file, 0, SEEK_SET) != 0)
	{
		printf("Could not seek in file.\n");

		return 1;
	}

	void* spv_data = malloc(input_bytes);

	if (fread(spv_data, 1, input_bytes, input_file) != input_bytes)
	{
		printf("Could not read from file.\n");

		return 1;
	}

	if (argc == 3)
	{
		if (fopen_s(&output_file, argv[2], "wb") != 0)
		{
			printf("Could not open file %s for writing.\n", argv[2]);

			return 1;
		}
	}

	uint32_t table_cnt = reinterpret_cast<const spird::file_header*>(spv_data)->table_count;

	for (uint32_t t = 0; t != table_cnt; ++t)
	{
		const spird::table_header* header = reinterpret_cast<const spird::table_header*>(static_cast<const uint8_t*>(spv_data) + sizeof(spird::file_header)) + t;

		fprintf(output_file, "\n%s : [\n", enum_name_strings[t]);

		const uint8_t* raw_data = static_cast<const uint8_t*>(spv_data);

		const spird::file_header* file_header = static_cast<const spird::file_header*>(spv_data);

		const spird::elem_index* indices = reinterpret_cast<const spird::elem_index*>(raw_data + header->offset);

		for (uint32_t i = 0; i != header->size; ++i)
		{
			uint32_t id = indices[i].id;

			if (id == ~0u)
				continue;

			spird::data_info elem_data;

			if (spvcpu::result rst = spird::get_data(spv_data, static_cast<spird::enum_id>(t), id, &elem_data); rst != spvcpu::result::success)
			{
				printf("Could not get operation data for id %d. (Error %d in table '%s')\n", id, rst, enum_name_strings[t]);

				return 1;
			}

			fprintf(output_file, "\t{\n\t\tid : %d\n\t\tname : \"%s\"\n", id, elem_data.name == nullptr ? "<Unknown>" : elem_data.name);

			if (elem_data.argc > 0)
				fprintf(output_file, "\t\targs : [\n");

			for (uint32_t i = 0; i != elem_data.argc; ++i)
			{
				uint8_t argtype = static_cast<uint8_t>(elem_data.arg_types[i]);

				const char* optstr = "";

				const char* varstr = "";

				if (argtype & spird::insn_arg_optional_bit)
					optstr = "OPT ";

				if (argtype & spird::insn_arg_variadic_bit)
					varstr = "VAR ";

				argtype &= spird::insn_argtype_mask;

				const char* argtypename = "<Invalid>";

				if (argtype < _countof(argument_type_names))
					argtypename = argument_type_names[argtype];

				fprintf(output_file, "\t\t\t%s%s%s", optstr, varstr, argtypename);

				if (elem_data.arg_names[i] == nullptr)
					fprintf(output_file, "\n");
				else
					fprintf(output_file, " \"%s\"\n", elem_data.arg_names[i]);
			}

			if (elem_data.argc > 0)
				fprintf(output_file, "\t\t]\n");

			if (elem_data.implies_or_depends != spird::implies_or_depends_mode::none)
			{
				fprintf(output_file, elem_data.implies_or_depends == spird::implies_or_depends_mode::depends ? "\t\tdepends : " : "\t\timplies : ");

				if (elem_data.capability_cnt > 1)
				{
					fprintf(output_file, "[\n");

					for (uint8_t i = 0; i != elem_data.capability_cnt; ++i)
					{
						const char* str = nullptr;

						for (uint32_t j = 0; j != _countof(capability_ids); ++j)
							if (elem_data.capabilities[i] == capability_ids[j])
							{
								str = capability_name_strings[j];

								break;
							}

						fprintf(output_file, "\t\t\t%s\n", str);
					}

					fprintf(output_file, "\t\t]\n");
				}
				else
				{
					const char* str = nullptr;

					for (uint32_t j = 0; j != _countof(capability_ids); ++j)
						if (elem_data.capabilities[0] == capability_ids[j])
						{
							str = capability_name_strings[j];

							break;
						}

					fprintf(output_file, "%s\n", str);
				}
			}

			fprintf(output_file, "\t}\n");
		}

		fprintf(output_file, "]\n");
	}

	return 0;
}
