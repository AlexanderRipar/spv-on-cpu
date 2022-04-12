#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "spird_accessor.hpp"
#include "spird_names.hpp"
#include "spv_viewer.hpp"

#ifdef _WIN32
#define ftell _ftelli64
#define fseek _fseeki64
#endif

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

bool get_file_content(const char* filename, void** out_data, uint64_t* out_bytes) noexcept
{
	FILE* file;

	if (fopen_s(&file, filename, "rb") != 0)
	{
		fprintf(stderr, "Could not open file '%s'.\n", filename);

		return false;
	}

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fprintf(stderr, "Could not seek in file '%s'.\n", filename);

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	const int64_t bytes = ftell(file);

	if (bytes < 0)
	{
		fprintf(stderr, "Could not ftell in file '%s'.\n", filename);

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	if (fseek(file, 0, SEEK_SET) != 0)
	{
		fprintf(stderr, "Could not seek in file '%s'.\n", filename);

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	void* buffer = malloc(bytes);

	if (buffer == nullptr)
	{
		fprintf(stderr, "malloc failed.\n");

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	if (fread(buffer, 1, bytes, file) != bytes)
	{
		fprintf(stderr, "Could not read from file '%s'.\n", filename);

		if (filename != nullptr)
			fclose(file);

		return false;
	}

	if (filename != nullptr)
		fclose(file);

	*out_data = buffer;

	*out_bytes = static_cast<uint64_t>(bytes);

	return true;
}

int cycle(int argc, const char** argv) noexcept
{
	if (argc != 2 && argc != 3)
	{
		fprintf(stderr, "Usage: %s input-file [output-file]\n", argv[0]);

		return 0;
	}

	void* spird;

	uint64_t spird_bytes;

	if (!get_file_content(argv[1], &spird, &spird_bytes))
		return 1;
		
	FILE* output_file = stdout;

	if (argc == 3)
	{
		if (fopen_s(&output_file, argv[2], "wb") != 0)
		{
			fprintf(stderr, "Could not open file %s for writing.\n", argv[2]);

			return 1;
		}
	}

	spird::named_enum_data named_data;

	if (spvcpu::result rst = spird::get_named_enum_data(spird, &named_data); rst != spvcpu::result::success)
	{
		fprintf(stderr, "Could not get data on named enumerations. (Error %d)\n", rst);

		return 1;
	}

	const uint32_t unnamed_table_cnt = reinterpret_cast<const spird::file_header*>(spird)->unnamed_table_count;

	const uint32_t named_table_cnt = named_data.name_count;

	// This works because the headers for unnamed and named tables are located directly after each other
	const uint32_t table_cnt = unnamed_table_cnt + named_table_cnt;

	for (uint32_t t = 0; t != table_cnt; ++t)
	{
		spird::enum_location enum_loc;

		if (t < unnamed_table_cnt)
		{
			if (spvcpu::result rst = spird::get_enum_location(spird, static_cast<spird::enum_id>(t), &enum_loc); rst != spvcpu::result::success)
			{
				fprintf(stderr, "Could not locate enumeration %d. (Error %d)\n", t, rst);

				return 1;
			}
		}
		else
		{
			if (spvcpu::result rst = spird::get_enum_location(spird, named_data.names[t - unnamed_table_cnt], &enum_loc); rst != spvcpu::result::success)
			{
				fprintf(stderr, "Could not locate enumeration %s. (Error %d)\n", named_data.names[t - unnamed_table_cnt], rst);

				return 1;
			}
		}

		spird::enum_data enum_data;

		if (spvcpu::result rst = spird::get_enum_data(spird, enum_loc, &enum_data); rst != spvcpu::result::success)
		{
			fprintf(stderr, "Could not get data for enumeration %d. (Error %d)\n", t, rst);

			return 1;
		}

		fprintf(output_file, "\n%s", enum_data.name);

		if ((enum_data.flags & spird::enum_flags::bitmask) == spird::enum_flags::bitmask)
			fprintf(output_file, "@BITMASK");

		if ((enum_data.flags & spird::enum_flags::named) == spird::enum_flags::named)
			fprintf(output_file, "@NAMED");

		fprintf(output_file, " : [\n");

		const uint8_t* raw_data = static_cast<const uint8_t*>(spird);

		const spird::file_header* file_header = static_cast<const spird::file_header*>(spird);

		const spird::table_header* table_header = reinterpret_cast<const spird::table_header*>(raw_data + file_header->first_table_header_byte) + t;

		const spird::elem_index* indices = reinterpret_cast<const spird::elem_index*>(raw_data + table_header->offset);

		for (uint32_t i = 0; i != table_header->size; ++i)
		{
			uint32_t id = indices[i].id;

			if (id == ~0u)
				continue;

			spird::elem_data elem_data;

			if (spvcpu::result rst = spird::get_elem_data(spird, enum_loc, id, &elem_data); rst != spvcpu::result::success)
			{
				fprintf(stderr, "Could not get data for element %d of enumeration '%s' (%d). (Error %d)\n", id, enum_data.name, t, rst);

				return 1;
			}

			if ((enum_data.flags & spird::enum_flags::bitmask) == spird::enum_flags::bitmask)
				fprintf(output_file, "\t{\n\t\tid : 0x%x", id);
			else
				fprintf(output_file, "\t{\n\t\tid : %d", id);

			fprintf(output_file, "\n\t\tname : \"%s\"\n", elem_data.name == nullptr ? "<Unknown>" : elem_data.name);

			if (elem_data.argc > 0)
				fprintf(output_file, "\t\targs : [\n");

			for (uint32_t i = 0; i != elem_data.argc; ++i)
			{
				spird::arg_type arg_type = elem_data.arg_types[i];

				spird::arg_flags arg_flags = elem_data.arg_flags[i];

				const char* optstr = "";

				const char* varstr = "";

				const char* idstr = "";

				const char* pairstr = "";

				if ((arg_flags & spird::arg_flags::optional) == spird::arg_flags::optional)
					optstr = "OPT ";

				if ((arg_flags & spird::arg_flags::variadic) == spird::arg_flags::optional)
					varstr = "VAR ";

				if ((arg_flags & spird::arg_flags::id) == spird::arg_flags::id)
					idstr = "ID ";

				if ((arg_flags & spird::arg_flags::pair) == spird::arg_flags::pair)
					pairstr = "PAIR ";

				const char* type_name;

				if (!spird::get_name_from_arg_type(arg_type, &type_name))
				{
					fprintf(stderr, "Could not get name of arg_type %d.\n", arg_type);

					return 1;
				}

				if ((arg_flags & spird::arg_flags::result) == spird::arg_flags::result)
				{
					fprintf(output_file, "\t\t\tRST %s\n", type_name);
				}
				else
				{
					fprintf(output_file, "\t\t\t%s%s%s%s", optstr, varstr, idstr, type_name);

					if (elem_data.arg_names[i] == nullptr)
						fprintf(output_file, "\n");
					else
						fprintf(output_file, " \"%s\"\n", elem_data.arg_names[i]);
				}
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

int disasm(int argc, const char** argv) noexcept
{
	if (argc < 3 || argc > 5)
	{
		printf("Usage: %s [--types] shader-file spird-file [output-file]\n", argv[0]);

		return 0;
	}

	bool print_type_info = false;

	uint32_t curr_arg = 1;

	if (strcmp(argv[1], "--types") == 0)
	{
		print_type_info = true;

		curr_arg = 2;
	}

	FILE* output_file = stdout;


	uint64_t shader_bytes;

	void* shader_data;

	uint64_t spird_bytes;

	void* spird_data;

	if (!get_file_content(argv[curr_arg++], &shader_data, &shader_bytes))
		return 1;

	if (!get_file_content(argv[curr_arg++], &spird_data, &spird_bytes))
		return 1;

	if (curr_arg != argc)
	{
		if (fopen_s(&output_file, argv[curr_arg], "w") != 0)
		{
			fprintf(stderr, "Could not open file %s for writing.\n", argv[2]);

			return 1;
		}
	}

	char* disassembly;

	uint64_t disassembly_bytes;

	if (spvcpu::result rst = spvcpu::disassemble(shader_bytes, shader_data, spird_data, print_type_info, &disassembly_bytes, &disassembly); rst != spvcpu::result::success)
	{
		fprintf(stderr, "spvcpu::disassemble failed with error %d.\n", static_cast<uint32_t>(rst));

		return 1;
	}

	if (fwrite(disassembly, 1, disassembly_bytes - 1, output_file) != disassembly_bytes - 1)
	{
		fprintf(stderr, "Could not write to %s%s", output_file == stdout ? "" : "file ", output_file == stdout ? "stdout" : argv[3]);

		return 1;
	}

	return 0;
}

void print_usage(const char* prog_name) noexcept
{
	fprintf(stderr, "Usage: %s (--cycle|--disasm) [additional args...]\n", prog_name);
}

int main(int argc, const char** argv)
{
	if (argc < 2)
	{
		print_usage(argv[0]);

		return 0;
	}
	else if (strcmp(argv[1], "--cycle") == 0)
	{
		return cycle(argc - 1, argv + 1);
	}
	else if (strcmp(argv[1], "--disasm") == 0)
	{
		return disasm(argc - 1, argv + 1);
	}
	else
	{
		print_usage(argv[0]);

		return 0;
	}

}
