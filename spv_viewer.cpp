#include "spv_viewer.hpp"

#include <cstdint>
#include <cstdlib>
#include <memory>

#include "spv_defs.hpp"
#include "spird_defs.hpp"
#include "spird_accessor.hpp"

struct output_buffer
{
private:

	static constexpr const char id_prefix = '$';

	char* m_string;
	uint32_t m_used;
	uint32_t m_capacity;

	bool grow(uint32_t additional) noexcept
	{
		if (m_used + additional > m_capacity)
		{
			char* tmp = static_cast<char*>(malloc(m_capacity * 2));

			if (tmp == nullptr)
				return false;

			m_capacity *= 2;

			m_string = tmp;
		}

		return true;
	}

	bool append_number_(uint32_t n) noexcept
	{
		uint32_t log10_ceil = 1;

		uint32_t approx = 10;

		while (n >= approx)
		{
			approx *= 10;

			++log10_ceil;
		}
		
		if (!grow(log10_ceil))
			return false;

		for(uint32_t i = 0; i != log10_ceil; ++i)
		{
			m_string[m_used + log10_ceil - i - 1] = (n % 10) + '0';

			n /= 10;
		}

		m_used += log10_ceil;

		return true;
	}

	bool append_string_(const char* str) noexcept
	{
		uint32_t additional = static_cast<uint32_t>(strlen(str));

		if (!grow(additional))
			return false;

		memcpy(m_string + m_used, str, additional);

		m_used += additional;

		return true;
	}

public:

	output_buffer() noexcept : m_string{ nullptr }, m_used{ 0 }, m_capacity{ 0 } {}

	~output_buffer() noexcept
	{
		free(m_string);
	}

	spvcpu::result initialize() noexcept
	{
		m_string = static_cast<char*>(malloc(4096));

		if (m_string == nullptr)
			return spvcpu::result::no_memory;

		m_capacity = 4096;

		return spvcpu::result::success;
	}

	spvcpu::result finalize() noexcept
	{
		if (!grow(1))
			return spvcpu::result::no_memory;

		m_string[m_used++] = '\0';

		return spvcpu::result::success;
	}

	spvcpu::result append_result(uint32_t id, uint32_t typid) noexcept
	{
		if (!append_string_("$") || !append_number_(typid))
			return spvcpu::result::no_memory;

		if (typid != ~0u)
			if (!append_string_(" ($") || !append_number_(typid) || !append_string_(")"))
				return spvcpu::result::no_memory;

		if (!append_string_(" = "))
			return spvcpu::result::no_memory;

		return spvcpu::result::success;
	}

	spvcpu::result append_instruction_name(const char* name) noexcept
	{
		if (!append_string_(name))
			return spvcpu::result::no_memory;

		return spvcpu::result::success;
	}

	spvcpu::result append_enum(const char* name, spirv_enum_id enum_id, uint32_t value) noexcept
	{
		enum_id; value;

		if (!append_string_(" [") || !append_string_(name) || !append_string_("]"))
			return spvcpu::result::no_memory;

		return spvcpu::result::success;
	}

	spvcpu::result append_id(uint32_t id) noexcept
	{
		if (!append_string_(" $") || !append_number_(id))
			return spvcpu::result::no_memory;

		return spvcpu::result::success;
	}

	spvcpu::result append_member(uint32_t member) noexcept
	{
		if (!append_string_(" @") || !append_number_(member))
			return spvcpu::result::no_memory;

		return spvcpu::result::success;
	}

	spvcpu::result append_u32(uint32_t n) noexcept
	{
		if (!append_string_(" ") || !append_number_(n))
			return spvcpu::result::no_memory;

		return spvcpu::result::success;
	}

	spvcpu::result append_string(const char* str) noexcept
	{
		if (!append_string_(str))
			return spvcpu::result::no_memory;

		return spvcpu::result::success;
	}
};

struct deleter
{
	void operator()(uint32_t* ptr) noexcept
	{
		free(ptr);
	}
};

struct spirv_header
{
	uint32_t magic;
	uint32_t version;
	uint32_t generator_magic;
	uint32_t id_bound;
	uint32_t reserved_zero;
};

static constexpr uint32_t reverse_endianness(uint32_t n) noexcept
{
	return ((n >> 24) & 0x000000FF) | ((n >> 8) & 0x0000FF00) | ((n << 8) & 0x00FF0000) | ((n << 24) & 0xFF000000);
}

static spvcpu::result check_header(const void* shader_data) noexcept
{
	const spirv_header* header = static_cast<const spirv_header*>(shader_data);

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


// TODO: Handle bitmask enums
// TODO: Handle enums with arguments
__declspec(dllexport) spvcpu::result spvcpu::show_spirv(
	uint32_t shader_bytes,
	const void* shader_data,
	const void* instruction_data,
	char** out_disassembly
) noexcept
{
	const uint32_t* shader_words = static_cast<const uint32_t*>(shader_data);

	std::unique_ptr<uint32_t, deleter> copied_shader_data;

	if (result header_result = check_header(shader_words); header_result == result::wrong_endianness)
	{
		copied_shader_data = std::unique_ptr<uint32_t, deleter>(static_cast<uint32_t*>(malloc(shader_bytes)));

		if (copied_shader_data == nullptr)
			return result::no_memory;

		for (uint32_t i = 0; i != shader_bytes / 4; ++i)
			copied_shader_data.get()[i] = reverse_endianness(shader_words[i]);
		
		shader_words = copied_shader_data.get();

		if (result header_result = check_header(shader_words); header_result != result::success)
			return header_result;
	}
	else if (header_result != result::success)
		return header_result;

	output_buffer output;

	for(uint32_t word_index = 5; word_index < shader_bytes / 4;)
	{
		uint32_t wordcount = shader_words[word_index] >> 16;

		uint32_t opcode = shader_words[word_index] & 0xFFFF;

		spirv_data_info op_data;

		if (result op_result = get_spirv_data(instruction_data, spirv_enum_id::Instruction, opcode, &op_data); op_result != result::success)
			return op_result;

		uint16_t arg_inds[256];

		uint32_t operand_word_index = 1;

		uint32_t present_argc = 0;

		uint32_t result_index = ~0u;

		uint32_t result_type_index = ~0u;

		for(uint32_t i = 0; i != op_data.argc; ++i)
		{
			bool is_optional = static_cast<uint8_t>(op_data.arg_types[i]) & spirv_insn_arg_optional_bit;

			bool is_variadic = static_cast<uint8_t>(op_data.arg_types[i]) & spirv_insn_arg_variadic_bit;

			if (operand_word_index == wordcount)
			{
				if (is_optional)
					break;
				else
					return result::instruction_wordcount_mismatch;
			}
			else if (operand_word_index > wordcount)
				return result::instruction_wordcount_mismatch;

			++present_argc;

			spirv_insn_argtype arg_type = static_cast<spirv_insn_argtype>(static_cast<uint8_t>(op_data.arg_types[i]) & spirv_insn_argtype_mask);

			arg_inds[i] = static_cast<uint16_t>(operand_word_index);

			switch (arg_type)
			{
			case spirv_insn_argtype::RST:
			{
				result_index = operand_word_index;

				++operand_word_index;

				break;
			}
			case spirv_insn_argtype::RTYPE:
			{
				result_type_index = operand_word_index;

				++operand_word_index;

				break;
			}
			case spirv_insn_argtype::ID:
			case spirv_insn_argtype::TYPID:
			case spirv_insn_argtype::U32:
			case spirv_insn_argtype::MEMBER:
			{
				++operand_word_index;

				break;
			}
			case spirv_insn_argtype::LITERAL:
			{
				operand_word_index += static_cast<uint32_t>(wordcount - 1);

				break;
			}
			case spirv_insn_argtype::STR:
			{
				const char* str = reinterpret_cast<const char*>(shader_words + word_index + operand_word_index);

				operand_word_index += static_cast<uint16_t>((strlen(str) + 4) >> 2);

				break;
			}
			case spirv_insn_argtype::ARG:
			{
				// ARG only ever comes at the end of an instruction, 
				// so we can just assume that this completes the instruction wordcount

				operand_word_index += static_cast<uint32_t>(wordcount - 1);

				break;
			}
			case spirv_insn_argtype::U32IDPAIR:
			case spirv_insn_argtype::IDMEMBERPAIR:
			case spirv_insn_argtype::IDIDPAIR:
			{
				operand_word_index += 2;

				break;
			}
			default: // Enumeration
			{
				++operand_word_index;

				break;
			}
			}
		}



		if (result_index != ~0u)
		{
			uint32_t result_id = shader_words[word_index + result_index];

			uint32_t result_type_id = result_type_index == ~0u ? ~0u : shader_words[word_index + result_type_index];

			if (result rst = output.append_result(result_id, result_type_id); rst != result::success)
				return rst;
		}

		if (result rst = output.append_instruction_name(op_data.name); rst != result::success)
			return rst;

		for (uint32_t i = 0; i != present_argc; ++i)
		{
			const void* arg_ptr = shader_words + word_index + arg_inds[i];

			uint8_t argtype_raw = static_cast<uint8_t>(op_data.arg_types[i]);

			bool is_variadic = argtype_raw & spirv_insn_arg_variadic_bit;

			argtype_raw &= spirv_insn_argtype_mask;

			if (argtype_raw < spirv_enum_id_count)
			{
				// TODO: Enums with arguments
				// TODO: Enums with bitmasks

				spirv_data_info arg_data;

				uint32_t value = static_cast<const uint32_t*>(arg_ptr)[0];

				if (result rst = get_spirv_data(instruction_data, static_cast<spirv_enum_id>(argtype_raw), value, &arg_data); rst != result::success)
					return rst;

				if (result rst = output.append_enum(arg_data.name, static_cast<spirv_enum_id>(argtype_raw), value); rst != result::success)
					return rst;

				if (is_variadic)
				{
					for(uint32_t j = 1; j != wordcount - arg_inds[i]; ++j)
					{
						uint32_t var_value = static_cast<const uint32_t*>(arg_ptr)[i];

						if (result rst = get_spirv_data(instruction_data, static_cast<spirv_enum_id>(argtype_raw), var_value, &arg_data); rst != result::success)
							return rst;

						if (result rst = output.append_enum(arg_data.name, static_cast<spirv_enum_id>(argtype_raw), var_value); rst != result::success)
							return rst;
					}
				}
			}
			else
			{
				uint32_t end_cond;

				if (is_variadic || i == present_argc - 1)
					end_cond = wordcount - arg_inds[present_argc - 1];
				else
					end_cond = arg_inds[i + 1] - arg_inds[i];

				uint32_t n = 0;

				while (n < end_cond)
				{
					switch (static_cast<spirv_insn_argtype>(argtype_raw))
					{
					case spirv_insn_argtype::RST:
					case spirv_insn_argtype::RTYPE:
					case spirv_insn_argtype::ARG:
					{
						++n;

						break;
					}
					case spirv_insn_argtype::U32:
					case spirv_insn_argtype::LITERAL:
					{
						// TODO: Non-32-bit Literals

						if (result rst = output.append_u32(static_cast<const uint32_t*>(arg_ptr)[n]); rst != result::success)
							return rst;

						++n;

						break;
					}
					case spirv_insn_argtype::ID:
					case spirv_insn_argtype::TYPID:
					{
						if (result rst = output.append_id(static_cast<const uint32_t*>(arg_ptr)[n]); rst != result::success)
							return rst;

						++n;

						break;
					}
					case spirv_insn_argtype::STR:
					{
						const char* str = reinterpret_cast<const char*>(static_cast<const uint32_t*>(arg_ptr) + n);

						if (result rst = output.append_string(str); rst != result::success)
							return rst;

						n += (strlen(str) + 4) >> 2;

						break;
					}
					case spirv_insn_argtype::MEMBER:
					{
						if (result rst = output.append_member(static_cast<const uint32_t*>(arg_ptr)[n]); rst != result::success)
							return rst;

						++n;

						break;
					}
					case spirv_insn_argtype::U32IDPAIR:
					{
						if (result rst = output.append_u32(static_cast<const uint32_t*>(arg_ptr)[n]); rst != result::success)
							return rst;

						if (result rst = output.append_id(static_cast<const uint32_t*>(arg_ptr)[n + 1]); rst != result::success)
							return rst;

						n += 2;

						break;
					}
					case spirv_insn_argtype::IDMEMBERPAIR:
					{
						if (result rst = output.append_id(static_cast<const uint32_t*>(arg_ptr)[n]); rst != result::success)
							return rst;

						if (result rst = output.append_member(static_cast<const uint32_t*>(arg_ptr)[n + 1]); rst != result::success)
							return rst;

						n += 2;

						break;
					}
					case spirv_insn_argtype::IDIDPAIR:
					{
						if (result rst = output.append_id(static_cast<const uint32_t*>(arg_ptr)[n]); rst != result::success)
							return rst;

						if (result rst = output.append_id(static_cast<const uint32_t*>(arg_ptr)[n + 1]); rst != result::success)
							return rst;

						n += 2;

						break;
					}
					default:
					{
						return result::unknown_argtype;
					}
					}
				}

				if (n != end_cond)
					return spvcpu::result::instruction_wordcount_mismatch;
			}
		}

		if (operand_word_index != wordcount)
			return result::instruction_wordcount_mismatch;

		output.append_string("\n");

		word_index += wordcount;
	}
	

	return result::success;
}
