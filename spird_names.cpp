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

bool spird::get_name_from_arg_type(spird::arg_type type, const char** out_name) noexcept
{
	if (static_cast<uint8_t>(type) < sizeof(arg_type_names_low_part) / sizeof(*arg_type_names_low_part))
	{
		*out_name = arg_type_names_low_part[static_cast<uint8_t>(type)];

		return true;
	}

	if (static_cast<uint8_t>(type) >= 256 - sizeof(arg_type_names_high_part) / sizeof(*arg_type_names_high_part))
	{
		*out_name = arg_type_names_high_part[static_cast<uint8_t>(type) - 216];

		return true;
	}

	return false;
}

bool spird::get_arg_type_from_name(const char* name, uint32_t bytes, spird::arg_type* out_type) noexcept
{
	for (uint32_t i = 0; i != sizeof(arg_type_names_low_part) / sizeof(*arg_type_names_low_part); ++i)
		if (bytes == 0)
		{
			if (strlen(arg_type_names_low_part[i]) == strlen(name) && strcmp(arg_type_names_low_part[i], name) == 0)
			{
				*out_type = static_cast<spird::arg_type>(i);

				return true;
			}
		}
		else
		{
			if (strlen(arg_type_names_low_part[i]) == bytes && strncmp(arg_type_names_low_part[i], name, bytes) == 0)
			{
				*out_type = static_cast<spird::arg_type>(i);

				return true;
			}
		}

	for (uint32_t i = 0; i != sizeof(arg_type_names_high_part) / sizeof(*arg_type_names_high_part); ++i)
		if (bytes == 0)
		{
			if (strlen(arg_type_names_high_part[i]) == strlen(name) && strcmp(arg_type_names_high_part[i], name) == 0)
			{
				*out_type = static_cast<spird::arg_type>(256 - sizeof(arg_type_names_high_part) / sizeof(*arg_type_names_high_part) + i);

				return true;
			}
		}
		else
		{
			if (strlen(arg_type_names_high_part[i]) == bytes && strncmp(arg_type_names_high_part[i], name, bytes) == 0)
			{
				*out_type = static_cast<spird::arg_type>(256 - sizeof(arg_type_names_high_part) / sizeof(*arg_type_names_high_part) + i);

				return true;
			}
		}

	return false;
}
