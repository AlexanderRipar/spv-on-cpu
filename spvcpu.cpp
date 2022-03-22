#include "spvcpu.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <memory>

#include <vulkan/vulkan.h>

#include "spv_defs.hpp"
#include "spv_helpers.hpp"

struct show_spirv_output
{
private:

	char* m_string = nullptr;
	
	uint32_t m_used = 0;
	
	uint32_t m_capacity = 0;

	bool m_has_mem = false;

	bool grow(uint32_t additional) noexcept
	{
		if(additional + m_used >= m_capacity)
		{
			m_capacity *= 2;

			char* tmp = static_cast<char*>(realloc(m_string, m_capacity));

			if(tmp == nullptr)
			{
				m_has_mem = false;

				return false;
			}

			m_string = tmp;
		}

		return m_has_mem;
	}

public:

	bool initialize() noexcept
	{
		m_string = static_cast<char*>(malloc(1024));

		if (m_string == nullptr)
			return false;

		m_string[0] = '\0';

		m_used = 0;

		m_capacity = 1024;

		m_has_mem = true;

		return true;
	}

	~show_spirv_output() noexcept
	{
		free(m_string);
	}

	char* steal() noexcept
	{
		char* tmp = m_string;

		m_string = nullptr;

		return tmp;
	}

	void append(const char* s) noexcept
	{
		const size_t len = strlen(s);

		if (grow(len))
		{
			memcpy(m_string + m_used, s, len);

			m_used += len;

			m_string[m_used] = '\0';
		}
	}

	void append(char c) noexcept
	{
		if (grow(1))
		{
			m_string[m_used++] = c;

			m_string[m_used] = '\0';
		}
	}

	void append(uint32_t n) noexcept
	{
		uint32_t len = 1;

		while (n >= 10000)
		{
			len += 4;

			n /= 10000;
		}

		if (n >= 1000)
			len += 3;
		else if (n >= 100)
			len += 2;
		else if (n > 10)
			len += 1;

		if (grow(len))
		{
			for (uint32_t i = len; i != 0; --i)
			{
				m_string[m_used + i] = '0' + static_cast<char>(n % 10);
		
				n /= 10;
			}

			m_string[m_used] = '0' + static_cast<char>(n);

			m_used += len;
		}
	}

	void append(int32_t n) noexcept
	{
		if (n < 0)
		{
			append('-');

			n = -n;
		}

		append(static_cast<uint32_t>(n));
	}

	void append(float f) noexcept
	{
		uint32_t pre_decimal;

		if(f < 0)
			pre_decimal = static_cast<uint32_t>(log10f(-f)) + 2;
		else
			pre_decimal = static_cast<uint32_t>(log10f(f)) + 1;

		uint32_t total = pre_decimal + 8; // 6 decimal places and decimal point. Null terminator gets included by grow

		if (!grow(total))
			return;

		int32_t written = snprintf(m_string + m_used, total + 1, "%.6f", f);

		if (written > static_cast<int32_t>(total) || written < 0)
			fprintf(stderr, "Error in append(float)! snprintf returned %d. Expected result in [0, %d].\n", written, total);
		else
			m_used += static_cast<uint32_t>(written);

		m_string[m_used] = '\0';
	}
};

static constexpr uint32_t reverse_endianness(uint32_t n) noexcept
{
	return ((n >> 24) & 0x000000FF) | ((n >> 8) & 0x0000FF00) | ((n << 8) & 0x00FF0000) | ((n << 24) & 0xFF000000);
}

static spvcpu::result check_header(const spirv_header* header) noexcept
{
	if (header->magic == reverse_endianness(spirv::magic_number))
		return spvcpu::result::wrong_endianness;

	if (header->magic != spirv::magic_number)
		return spvcpu::result::wrong_magic;

	if (header->version != spirv::version_1_0 && 
	    header->version != spirv::version_1_1 && 
	    header->version != spirv::version_1_2 && 
	    header->version != spirv::version_1_3 && 
	    header->version != spirv::version_1_4 &&
	    header->version != spirv::version_1_5)
		return spvcpu::result::invalid_spirv_version;

	if (header->id_bound > spirv::max_id_bound)
		return spvcpu::result::too_many_ids;

	if (header->reserved_zero != 0)
		return spvcpu::result::cannot_handle_header_schema;

	return spvcpu::result::success;
}

static const char* string_arg(const uint32_t* argv, uint32_t argc, uint32_t index, uint32_t* out_chars) noexcept
{
	const char* string = reinterpret_cast<const char*>(argv + index);

	size_t len = strnlen(string, (argc - index) * 4);

	if (len == (argc - index) * 4)
		return nullptr;

	*out_chars = len;

	return string;
}

static uint32_t strlen_to_wordcount(uint32_t len) noexcept
{
	return (len + 4) >> 2;
}



static spvcpu::result parse_instruction(show_spirv_output output, Op opcode, uint32_t argc, const uint32_t* argv) noexcept
{
	switch (opcode)
	{
	case Op::Nop:
	{
		if (argc != 0)
			return spvcpu::result::instruction_wordcount_mismatch;

		output.append("Nop\n");

		return spvcpu::result::success;
	}
	case Op::Undef:
	{
		if (argc != 1)
			return spvcpu::result::instruction_wordcount_mismatch;

		output.append('$');

		output.append(argv[1]);

		output.append(" = Undef [RType:$");

		output.append(argv[0]);

		output.append("]\n");

		return spvcpu::result::success;
	}
	case Op::SizeOf:
	{
		if (argc != 3)
			return spvcpu::result::instruction_wordcount_mismatch;

		output.append('$');

		output.append(argv[1]);

		output.append(" = SizeOf [RType:$");

		output.append(argv[0]);

		output.append(" Ptr:$");

		output.append(argv[2]);

		output.append("]\n");

		return spvcpu::result::success;
	}
	case Op::AssumeTrueKHR:
	{
		if (argc != 1)
			return spvcpu::result::instruction_wordcount_mismatch;

		output.append("AssumeTrueKHR [Condition:$");

		output.append(argv[0]);

		output.append("]\n");

		return spvcpu::result::success;
	}
	case Op::ExpectKHR:
	{
		if (argc != 4)
			return spvcpu::result::instruction_wordcount_mismatch;

		output.append('$');

		output.append(argv[1]);

		output.append(" = ExpectKHR [RType:");

		output.append(argv[0]);

		output.append(" Value:");

		output.append(argv[2]);

		output.append(" ExpectedValue:");

		output.append(argv[3]);

		output.append("]\n");

		return spvcpu::result::success;
	}
	case Op::SourceContinued:
	{
		if(argc < 1)
			return spvcpu::result::instruction_wordcount_mismatch;

		uint32_t source_cont_len;

		const char* source_cont_str = string_arg(argv, argc, 0, &source_cont_len);

		if (source_cont_str == nullptr || strlen_to_wordcount(source_cont_len) + 1 != argc)
			return spvcpu::result::instruction_wordcount_mismatch;

		output.append("SourceContinued [\n");

		output.append(source_cont_str);

		output.append("\n]\n");

		return spvcpu::result::success;
	}
	case Op::Source:
	{
		if (argc < 2)
			return spvcpu::result::instruction_wordcount_mismatch;

		const char* file_str = nullptr, *source_str = nullptr;

		uint32_t file_len, source_len;

		if(argc > 2)
		{
			file_str = string_arg(argv, argc, 2, &file_len);

			if (file_str == nullptr)
				return spvcpu::result::instruction_wordcount_mismatch;

			uint32_t file_words = strlen_to_wordcount(file_len);

			if (file_words + 2 != argc)
			{
				source_str = string_arg(argv, argc, file_words + 2, &source_len);

				if (source_str == nullptr || file_words + strlen_to_wordcount(source_len) + 2 != argc)
					return spvcpu::result::instruction_wordcount_mismatch;
			}
		}
		
		const char* source_language = get_source_language_info(static_cast<SourceLanguage>(argv[0])).name;

		output.append("Source [Language:");

		output.append(source_language);

		output.append(" Version:");

		output.append(argv[1]);

		if(file_str != nullptr)
		{
			output.append(" File:");

			output.append(file_str);
		}
		if (source_str != nullptr)
		{
			output.append(" Source:\n");

			output.append(source_str);

			output.append('\n');
		}

		output.append("]\n");

		return spvcpu::result::success;
	}
	case Op::SourceExtension:
	{
		if (argc < 1)
			return spvcpu::result::instruction_wordcount_mismatch;

		uint32_t extension_len;

		const char* extension_str = string_arg(argv, argc, 0, &extension_len);

		if (extension_str == nullptr || strlen_to_wordcount(extension_len) != argc)
			return spvcpu::result::instruction_wordcount_mismatch;
			
		output.append("SourceExtension [Extension:");

		output.append(extension_str);

		output.append("]\n");

		return spvcpu::result::success;
	}
	case Op::Name:
	{
		if (argc < 2)
			return spvcpu::result::instruction_wordcount_mismatch;

		uint32_t name_len;

		const char* name_str = string_arg(argv, argc, 1, &name_len);

		if (strlen_to_wordcount(name_len) + 1 != argc)
			return spvcpu::result::instruction_wordcount_mismatch;

		output.append("Name [Target:$");

		output.append(argv[0]);

		output.append(" Name:");

		output.append(name_str);

		output.append("]\n");

		return spvcpu::result::success;
	}
	case Op::MemberName:
	{
		if (argc < 3)
			return spvcpu::result::instruction_wordcount_mismatch;

		uint32_t name_len;

		const char* name_str = string_arg(argv, argc, 2, &name_len);

		if (strlen_to_wordcount(name_len) + 2 != argc)
			return spvcpu::result::instruction_wordcount_mismatch;

		output.append("Name [Target:$");

		output.append(argv[0]);

		output.append("@");

		output.append(argv[1]);

		output.append(" Name:");

		output.append(name_str);

		output.append("]\n");

		return spvcpu::result::success;
	}
	case Op::String:
	{
		if (argc < 2)
			return spvcpu::result::instruction_wordcount_mismatch;

		uint32_t string_len;

		const char* string_str = string_arg(argv, argc, 1, &string_len);

		if (strlen_to_wordcount(string_len) + 1 != argc)
			return spvcpu::result::instruction_wordcount_mismatch;

		output.append("$");

		output.append(argv[0]);

		output.append(" = String [String:");

		output.append(string_str);

		output.append("]\n");

		return spvcpu::result::success;
	}
	case Op::Line:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::NoLine:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ModuleProcessed:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Decorate:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::MemberDecorate:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DecorationGroup:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupDecorate:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupMemberDecorate:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DecorateId:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DecorateString:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::MemberDecorateString:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Extension:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ExtInstImport:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ExtInst:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::MemoryModel:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::EntryPoint:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ExecutionMode:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Capability:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ExecutionModeId:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeVoid:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeBool:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeInt:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeFloat:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeVector:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeMatrix:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeImage:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeSampler:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeSampledImage:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeArray:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeRuntimeArray:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeStruct:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeOpaque:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypePointer:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeFunction:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeEvent:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeDeviceEvent:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeReserveId:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeQueue:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypePipe:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeForwardPointer:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypePipeStorage:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeNamedBarrier:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeBufferSurfaceINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeStructContinuedINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConstantTrue:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConstantFalse:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Constant:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConstantComposite:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConstantSampler:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConstantNull:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SpecConstantTrue:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SpecConstantFalse:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SpecConstant:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SpecConstantComposite:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SpecConstantOp:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConstantCompositeContinuedINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SpecConstantCompositeContinuedINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Variable:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageTexelPointer:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Load:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Store:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CopyMemory:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CopyMemorySized:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AccessChain:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::InBoundsAccessChain:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::PtrAccessChain:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ArrayLength:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GenericPtrMemSemantics:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::InBoundsPtrAccessChain:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::PtrEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::PtrNotEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::PtrDiff:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Function:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FunctionParameter:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FunctionEnd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FunctionCall:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SampledImage:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSampleImplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSampleExplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSampleDrefImplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSampleDrefExlicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSampleProjImplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSampleProjExplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSampleProjDrefImplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSampleProjDrefExplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageFetch:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageGather:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageDrefGather:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageRead:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageWrite:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Image:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageQueryFormat:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageQueryOrder:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageQuerySizeLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageQuerySize:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageQueryLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageQueryLevels:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageQuerySamples:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseSampleImplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseSampleExplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseSampleDrefImplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseSampleDrefExplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseSampleProjImplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseSampleProjExplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseSampleProjDrefImplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseSampleProjDrefExplicitLod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseFetch:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseGather:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseDrefGather:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseTexelsResident:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSparseRead:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ImageSampleFootprintNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertFToU:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertFToS:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertSToF:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertUToF:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UConvert:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SConvert:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FConvert:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::QuantizeToF16:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertPtrToU:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SatConvertSToU:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SatConvertUToS:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertUToPtr:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::PtrCastToGeneric:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GenericCastToPtr:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GenericCastToPtrExplicit:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Bitcast:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::VectorExtractDynamic:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::VectorInsertDynamic:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::VectorShuffle:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CompositeConstruct:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CompositeExtract:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CompositeInsert:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CopyObject:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Transpose:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CopyLogical:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SNegate:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FNegate:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IAdd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FAdd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ISub:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FSub:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IMul:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FMul:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UDiv:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SDiv:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FDiv:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UMod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SRem:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SMod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FRem:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FMod:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::VectorTimesScalar:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::MatrixTimesScalar:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::VectorTimesMatrix:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::MatrixTimesVector:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::MatrixTimesMatrix:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::OuterProduct:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Dot:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IAddCarry:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ISubBorrow:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UMulExtended:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SMulExtended:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SDot:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UDot:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SUDot:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SDotAccSat:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UDotAccSat:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SUDotAccSat:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ShiftRightLogical:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ShiftRightArithmetic:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ShiftLeftLogical:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BitwiseOr:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BitwiseXor:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BitwiseAnd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Not:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BitFieldInsert:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BitFieldSExtract:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BitFieldUExtract:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BitReverse:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BitCount:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Any:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::All:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IsNan:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IsInf:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IsFinite:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IsNormal:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SignBitSet:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LessOrGreater:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Ordered:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Unordered:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LogicalEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LogicalNotEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LogicalOr:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LogicalAnd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LogicalNot:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Select:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::INotEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UGreaterThan:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SGreaterThan:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UGreaterThanEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SGreaterThanEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ULessThan:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SLessThan:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ULessThanEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SLessThanEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FOrdEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FUnordEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FOrdNotEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FUnordNotEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FOrdLessThan:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FUnordLessThan:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FOrdGreaterThan:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FUnordGreaterThan:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FOrdLessThanEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FUnordLessThanEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FOrdGreaterThanEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FUnordGreaterThanEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DPdx:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DPdy:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Fwidth:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DPdxFine:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DPdyFine:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FwidthFine:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DPdxCoarse:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DPdyCoarse:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FwidthCoarse:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Phi:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LoopMerge:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SelectionMerge:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Label:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Branch:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BranchConditional:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Switch:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Kill:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Return:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReturnValue:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::Unreachable:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LifetimeStart:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LifetimeStop:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TerminateInvocation:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::DemoteToHelperInvocation:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicLoad:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicStore:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicExchange:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicCompareExchange:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicCompareExchangeWeak:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicIIncrement:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicIDecrement:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicIAdd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicISub:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicSMin:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicUMin:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicSMax:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicUMax:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicAnd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicOr:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicXor:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicFlagTestAndSet:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicFlagClear:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicFMinEXT:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::tomicFMaxEXT:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AtomicFAddEXT:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::EmitVertex:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::EndPrimitive:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::EmitStreamVertex:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::EndStreamPrimitive:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ControlBarrier:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::MemoryBarrier:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::NamedBarrierInitialize:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::MemoryNamedBarrier:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupAsyncCopy:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupWaitEvents:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupAll:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupAny:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupBroadcast:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupIAdd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupFAdd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupFMin:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupUMin:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupSMin:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupFMax:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupUMax:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupSMax:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupBallotKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupFirstInvocationKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupAllKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupAnyKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupAllEqualKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupReadInvocationKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupIAddNonUniformAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupFAddNonUniformAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupFMinNonUniformAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupUMinNonUniformAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupSMinNonUniformAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupFMaxNonUniformAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupUMaxNonUniformAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupSMaxNonUniformAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupShuffleINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupShuffleDownINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupShuffleUpINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupShuffleXorINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupBlockReadINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupBlockWriteINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupImageBlockReadINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupImageBlockWriteINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupImageMediaBlockReadINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SubgroupImageMediaBlockWriteINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::EnqueueMarker:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::EnqueueKernel:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GetKernelNDrangeSubGroupCount:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GetKernelNDrangeMaxSubGroupSize:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GetKernelWorkGroupSize:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GetKernelPreferredWorkGroupSizeMultiple:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RetainEvent:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReleaseEvent:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CreateUserEvent:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IsValidEvent:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SetUserEventStatus:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CaptureEventProfilingInfo:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GetDefaultQueue:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BuildNDRange:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GetKernelLocalSizeForSubgroupCount:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GetKernelMaxNumSubgroups:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReadPipe:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::WritePipe:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReservedReadPipe:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReservedWritePipe:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReserveReadPipePackets:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReserveWritePipePackets:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CommitReadPipe:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CommitWritePipe:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IsValidReserveId:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GetNumPipePackets:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GetMaxPipePackets:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupReserveReadPipePackets:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupReserveWritePipePackets:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupCommitReadPipe:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupCommitWritePipe:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConstantPipeStorage:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CreatePipeFromPipeStorage:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReadPipeBlockingINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::WritePipeBlockingINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformElect:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformAll:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformAny:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformAllEqual:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBroadcast:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBroadcastFirst:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBallot:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformInverseBallot:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBallotBitExtract:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBallotBitCount:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBallotFindLSB:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBallotFindMSB:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformShuffle:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformShuffleXor:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformShuffleUp:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformShuffleDown:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformIAdd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformFAdd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformIMul:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformFMul:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformSMin:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformUMin:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformFMin:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformSMax:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformUMax:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformFMax:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBitwiseAnd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBitwiseOr:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformBitwiseXor:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformLogicalAnd:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformLogicalOr:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformLogicalXor:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformQuadBroadcast:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformQuadSwap:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::GroupNonUniformPartitionNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TraceRayKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ExecuteCallableKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertUToAccelerationStructureKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IgnoreIntersectionKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TerminateRayKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeRayQueryKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryInitializeKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryTerminateKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGenerateIntersectionKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryConfirmIntersectionKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryProceedKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionTypeKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FragmentMaskFetchAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FragmentFetchAMD:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReadClockKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::WritePackedPrimitiveIndices4x8NV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ReportIntersectionNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IgnoreIntersectionNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TerminateRayNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TraceNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TraceMotionNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TraceRayMotionNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeAccelerationStructureNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ExecuteCallableNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::TypeCooperativeMatrixNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CooperativeMatrixLoadNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CooperativeMatrixStoreNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CooperativeMatrixMulAddNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::CooperativeMatrixLengthNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::BeginInvocationInterlockEXT:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::EndInvocationInterlockEXT:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IsHelperInvocationEXT:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertUToImageNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertUToSamplerNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertImageToUNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertSamplerToUNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertUToSampledImageNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ConvertSampledImageToUNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::SamplerImageAddressingModeNV:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UCountLeadingZerosINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UCountTrailingZerosINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AbsISubINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::AbsUSubINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IAddSatINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UAddSatINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IAverageINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UAverageINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IAverageRoundedINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UAverageRoundedINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::ISubSatINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::USubSatINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::IMul32x16INTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::UMul32x16INTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::LoopControlINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::FPGARegINTEL:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetRayTMinKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetRayFlagsKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionTKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionInstanceCustomIndexKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionInstanceIdKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionGeometryIndexKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionPrimitiveIndexKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionBarycentricsKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionFrontFaceKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionCandidateAABBOpaqueKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionObjectRayDirectionKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionObjectRayOriginKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetWorldRayDirectionKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetWorldRayOriginKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionObjectToWorldKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	case Op::RayQueryGetIntersectionWorldToObjectKHR:
	{
		return spvcpu::result::unhandled_opcode;
	}
	default:
	{
		return spvcpu::result::unhandled_opcode;
	}
	}

	return spvcpu::result::success;
}

__declspec(dllexport) spvcpu::result spvcpu::show_spirv(
	uint32_t shader_bytes,
	const void* raw_shader_data,
	char** out_disassembly
) noexcept
{
	const uint32_t* shader_data = static_cast<const uint32_t*>(raw_shader_data);
	
	struct deleter{ void operator()(uint32_t* p){ free(p); } };

	std::unique_ptr<uint32_t, deleter> allocated_shader_data;

	if (shader_bytes & 3)
		return result::shader_size_not_divisible_by_four;

	if (reinterpret_cast<uint64_t>(shader_data) & 3)
		return result::shader_data_misaligned;

	if (shader_bytes < sizeof(spirv_header))
		return result::shader_too_small;

	result header_result = check_header(reinterpret_cast<const spirv_header*>(shader_data));

	if (header_result == result::wrong_endianness)
	{
		allocated_shader_data = std::unique_ptr<uint32_t, deleter>(static_cast<uint32_t*>(malloc(shader_bytes * sizeof(uint32_t))));

		for(uint32_t i = 0; i != shader_bytes; ++i)
			allocated_shader_data.get()[i] = reverse_endianness(shader_data[i]);

		shader_data = allocated_shader_data.get();

		header_result = check_header(reinterpret_cast<const spirv_header*>(shader_data));
	}
	
	if (header_result != result::success)
	{
		return header_result;
	}

	const uint32_t total_words = shader_bytes / 4;

	uint32_t current_word = 5;

	show_spirv_output output;

	if (!output.initialize())
		return result::no_memory;

	while (current_word != total_words)
	{
		uint32_t wordcount = shader_data[current_word] >> 16;

		Op opcode = static_cast<Op>(shader_data[current_word] & 0xFFFF);

		if (current_word + wordcount > total_words)
			return result::instruction_past_data_end;

		result parse_result = parse_instruction(output, opcode, wordcount - 1, shader_data + current_word + 1);

		if (parse_result != result::success)
			return parse_result;
	}

	*out_disassembly = output.steal();

	return spvcpu::result::success;
}