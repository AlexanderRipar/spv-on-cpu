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

	static constexpr size_t max_u32_chars = 10;

	static constexpr size_t max_i32_chars = 11;

	static constexpr size_t max_u64_chars = 20;

	static constexpr size_t max_i64_chars = 20;

	char* m_string;
	uint32_t m_string_used;
	uint32_t m_string_capacity;

	char* m_line;
	uint32_t m_line_used;
	uint32_t m_line_capacity;

	uint32_t m_rst_id;
	uint32_t m_rtype_id;

	bool grow_string(size_t additional) noexcept
	{
		while (m_string_used + additional > m_string_capacity)
		{
			char* tmp = static_cast<char*>(malloc(m_string_capacity * 2));

			if (tmp == nullptr)
				return false;

			m_string_capacity *= 2;

			m_string = tmp;
		}

		return true;
	}

	bool grow_line(size_t additional) noexcept
	{
		while (m_line_used + additional > m_line_capacity)
		{
			char* tmp = static_cast<char*>(malloc(m_line_capacity * 2));

			if (tmp == nullptr)
				return false;

			m_line_capacity *= 2;

			m_line = tmp;
		}

		return true;
	}

	static void print_integer_to_buffer(char* out, uint32_t& out_used, uint64_t n, bool is_signed) noexcept
	{
		bool is_negative = false;

		if (is_signed && (n & (1ui64 << 63)))
		{
			is_negative = true;

			n = 0 - n;
		}

		uint32_t log10_ceil = 1;

		uint64_t approx = 10;

		while (n >= approx)
		{
			approx *= 10;

			++log10_ceil;
		}

		if (is_negative)
			out[out_used++] = '-';

		for(uint32_t i = 0; i != log10_ceil; ++i)
		{
			out[out_used + log10_ceil - i - 1] = (n % 10) + '0';

			n /= 10;
		}

		out_used += log10_ceil;
	}

	bool print_id(uint32_t id) noexcept
	{
		if (!print_str("$") || !grow_line(max_u32_chars))
			return false;
			
		print_integer_to_buffer(m_line, m_line_used, id, false);

		return true;
	}

	bool print_typid(uint32_t typid) noexcept
	{
		if (!print_str("T$") || !grow_line(max_u32_chars))
			return false;
			
			print_integer_to_buffer(m_line, m_line_used, typid, false);

		return true;
	}

	bool print_u32(uint32_t n) noexcept
	{
		if (!grow_line(max_u32_chars))
			return false;

		print_integer_to_buffer(m_line, m_line_used, n, false);

		return true;
	}

	bool print_str(const char* str) noexcept
	{
		const size_t len = strlen(str);

		if (!grow_line(len))
			return false;

		memcpy(m_line + m_line_used, str, len);

		m_line_used += len;

		return true;
	}

	bool print_member(uint32_t member) noexcept
	{
		if (!print_str("@") || !grow_line(max_u32_chars))
			return false;

		print_integer_to_buffer(m_line, m_line_used, member, false);

		return true;
	}

	bool print_i64(int64_t n) noexcept
	{
		if (!grow_line(max_i64_chars))
			return false;
		
		print_integer_to_buffer(m_line, m_line_used, n, true);

		return true;
	}

	bool str_print_rst_and_rtype() noexcept
	{
		// Factor in additional growth from writing rst and rtype or spaces.
		// Since ids are limited to at most 4194303, it is safe to assume that
		// 7 charcters for rst and rtype each, two additonal chars for spaces,
		// one for the rst marker, two for the rtype marker, and two for "= "
		// for a total of 7 + 7 + 2 + 1 + 2 + 2 = 21 should suffice for holding
		// the result prefix.
		constexpr uint32_t expected_prefix_len = 21;

		// To be conservative ensure that ints larger than the id-limit will
		// also fit.
		constexpr uint32_t conservative_prefix_len = expected_prefix_len + 6;

		if (!grow_string(conservative_prefix_len))
			return false;

		memset(m_string + m_string_used, ' ', expected_prefix_len);

		if (m_rst_id != ~0u)
		{
			m_string[m_string_used++] = '$';

			uint32_t rst_used = m_string_used;

			print_integer_to_buffer(m_string, rst_used, m_rst_id, false);

			if (rst_used < m_string_used + 7)
				rst_used = m_string_used + 7;

			m_string_used = rst_used;

			m_string[m_string_used++] = ' ';
		}
		else
		{
			m_string_used += 9;
		}

		if (m_rtype_id != ~0u)
		{
			m_string[m_string_used++] = 'T';

			m_string[m_string_used++] = '$';

			uint32_t rtype_used = m_string_used;

			print_integer_to_buffer(m_string, rtype_used, m_rtype_id, false);

			if (rtype_used < m_string_used + 7)
				rtype_used = m_string_used + 7;

			m_string_used = rtype_used;

			m_string[m_string_used++] = ' ';
		}
		else
		{
			m_string_used += 10;
		}

		if (m_rst_id != ~0u)
		{
			m_string[m_string_used++] = '=';

			m_string[m_string_used++] = ' ';
		}

		return true;
	}

	spvcpu::result print_enum(const void* spird, spird::enum_id enum_id, const uint32_t*& word, const uint32_t* word_end) noexcept
	{
		if (word == word_end)
			return spvcpu::result::instruction_wordcount_mismatch;

		const uint32_t* tmp_word = word + 1;

		spird::enum_data enum_data;
		
		if (spvcpu::result rst = spird::get_enum_data(spird, enum_id, &enum_data); rst != spvcpu::result::success)
			return rst;
		
		const uint32_t elem_id = *word;

		if ((enum_data.header->flags & spird::enum_flags::bitmask) == spird::enum_flags::bitmask)
		{
			uint32_t elem_id_bits = elem_id;

			while (elem_id_bits != 0)
			{
				uint32_t lsb = elem_id_bits & -elem_id_bits;

				elem_id_bits ^= lsb;

				spird::elem_data elem_data;

				if (spvcpu::result rst = spird::get_elem_data(spird, enum_id, lsb, &elem_data); rst != spvcpu::result::success)
					return rst;

				if (!print_str(elem_data.name))
					return spvcpu::result::no_memory;
			}

			elem_id_bits = elem_id;

			while (elem_id_bits != 0)
			{
				uint32_t lsb = elem_id_bits & -elem_id_bits;

				elem_id_bits ^= lsb;

				spird::elem_data elem_data;

				if (spvcpu::result rst = spird::get_elem_data(spird, enum_id, lsb, &elem_data); rst != spvcpu::result::success)
					return rst;

				for (uint32_t arg = 0; arg != elem_data.argc; ++arg)
					if (spvcpu::result rst = print_arg(spird, elem_data.arg_types[arg], tmp_word, word_end); rst != spvcpu::result::success)
						return rst;
			}
		}
		else
		{
			spird::elem_data elem_data;

			if (spvcpu::result rst = spird::get_elem_data(spird, enum_id, elem_id, &elem_data); rst != spvcpu::result::success)
				return rst;

			if (!print_str(elem_data.name))
				return spvcpu::result::no_memory;

			for (uint32_t arg = 0; arg != elem_data.argc; ++arg)
				if (spvcpu::result rst = print_arg(spird, elem_data.arg_types[arg], tmp_word, word_end); rst != spvcpu::result::success)
					return rst;
		}

		word = tmp_word;

		return spvcpu::result::success;
	}

public:

	output_buffer() noexcept : m_string{ nullptr }, m_line{ nullptr } {}

	~output_buffer() noexcept
	{
		free(m_string);
	}

	spvcpu::result initialize() noexcept
	{
		m_string = static_cast<char*>(malloc(4096));

		if (m_string == nullptr)
			return spvcpu::result::no_memory;

		m_string_used = 0;

		m_string_capacity = 4096;

		m_line = static_cast<char*>(malloc(1024));

		if (m_line == nullptr)
			return spvcpu::result::no_memory;

		m_line_used = 0;

		m_line_capacity = 4096;

		m_rst_id = ~0u;

		m_rtype_id = ~0u;

		return spvcpu::result::success;
	}

	spvcpu::result finalize() noexcept
	{
		if (!grow_string(1))
			return spvcpu::result::no_memory;

		m_string[m_string_used++] = '\0';

		return spvcpu::result::success;
	}

	char* steal() noexcept
	{
		char* tmp = m_string;

		m_string = nullptr;

		return tmp;
	}

	uint32_t size() const noexcept
	{
		return m_string_used;
	}

	spvcpu::result print_arg(const void* spird, spird::arg_type type, const uint32_t*& word, const uint32_t* word_end) noexcept
	{
		if (!print_str(" "))
			return spvcpu::result::no_memory;
		
		const bool is_optional_bit = static_cast<uint32_t>(type) & spird::insn_arg_optional_bit;

		const bool is_variadic = static_cast<uint32_t>(type) & spird::insn_arg_variadic_bit;

		type = static_cast<spird::arg_type>(static_cast<uint32_t>(type) & spird::insn_argtype_mask);

		if (is_optional_bit && word == word_end)
			return spvcpu::result::success;

		if (static_cast<uint32_t>(type) < spird::enum_id_count)
		{
			do
			{
				if (spvcpu::result rst = print_enum(spird, static_cast<spird::enum_id>(type), word, word_end); rst != spvcpu::result::success)
					return rst;
			}
			while (is_variadic && word < word_end);
		}
		else
		{
			do
			{
				switch (type)
				{
				case spird::arg_type::RST:
				{
					if (word + 1 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					m_rst_id = *word;

					++word;

					break;
				}
				case spird::arg_type::RTYPE:
				{
					if (word + 1 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					m_rtype_id = *word;

					++word;

					break;
				}
				case spird::arg_type::LITERAL:
				{
					// TODO
					word = word_end;

					break;
				}
				case spird::arg_type::VALUE:
				{
					if (word + 1 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					if (!print_id(*word))
						return spvcpu::result::no_memory;

					++word;

					break;
				}
				case spird::arg_type::TYPE:
				{
					if (word + 1 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					if (!print_typid(*word))
						return spvcpu::result::no_memory;

					++word;

					break;
				}
				case spird::arg_type::UNKNOWN:
				{
					// TODO

					break;
				}
				case spird::arg_type::U32:
				{
					if (word + 1 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					if (!print_u32(*word))
						return spvcpu::result::no_memory;

					++word; 

					break;
				}
				case spird::arg_type::STR:
				{
					const char* str = reinterpret_cast<const char*>(word);

					const size_t str_words = (strlen(str) + 4) >> 2;

					if (word + str_words > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					if (!print_str(str))
						return spvcpu::result::no_memory;

					word += str_words;

					break;
				}
				case spird::arg_type::ARG:
				{
					// ARG is handled implicitly by print_enum. Dont touch anything, 
					// just make sure we have consumed the complete instruction.

					if (word != word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					break;
				}
				case spird::arg_type::MEMBER:
				{
					if (word + 1 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					if (!print_member(*word))
						return spvcpu::result::no_memory;

					++word;

					break;
				}
				case spird::arg_type::U32IDPAIR:
				{
					if (word + 2 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					if (!print_u32(*word))
						return spvcpu::result::no_memory;

					if (!print_id(word[1]))
						return spvcpu::result::no_memory;

					word += 2;

					break;
				}
				case spird::arg_type::IDMEMBERPAIR:
				{
					if (word + 2 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					if (!print_id(*word))
						return spvcpu::result::no_memory;

					if (!print_member(word[1]))
						return spvcpu::result::no_memory;

					word += 2;

					break;
				}
				case spird::arg_type::IDIDPAIR:
				{
					if (word + 2 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					if (!print_id(*word))
						return spvcpu::result::no_memory;

					if (!print_id(word[1]))
						return spvcpu::result::no_memory;

					word += 2;

					break;
				}
				case spird::arg_type::IDU32PAIR:
				{
					if (word + 2 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					if (!print_id(*word))
						return spvcpu::result::no_memory;

					if (!print_u32(word[1]))
						return spvcpu::result::no_memory;

					word += 2;

					break;
				}
				case spird::arg_type::I64:
				{
					if (word + 2 > word_end)
						return spvcpu::result::instruction_wordcount_mismatch;

					int64_t n = *word | (static_cast<int64_t>(word[1]) << 32); 

					if (!print_i64(n))
						return spvcpu::result::no_memory;

					word += 2;

					break;
				}
				default:
				{
					return spvcpu::result::unknown_argtype;
				}
				}
			}
			while (is_variadic && word < word_end);
		}

		return spvcpu::result::success;
	}

	spvcpu::result print_instruction_name(const char* name) noexcept
	{
		if (!print_str("Op") || !print_str(name))
			return spvcpu::result::no_memory;

		return spvcpu::result::success;
	}

	spvcpu::result end_line() noexcept
	{
		if (!str_print_rst_and_rtype())
			return spvcpu::result::no_memory;

		// Don't forget to reserve space for the trailing '\n'
		if (!grow_string(m_line_used + 1))
			return spvcpu::result::no_memory;

		memcpy(m_string + m_string_used, m_line, m_line_used);

		m_string_used += m_line_used + 1;

		m_line_used = 0;

		m_rst_id = ~0u;

		m_rtype_id = ~0u;
		
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

static spvcpu::result check_header(const void* spirv) noexcept
{
	const spirv_header* header = static_cast<const spirv_header*>(spirv);

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
	uint64_t spirv_bytes,
	const void* spirv,
	const void* spird,
	uint64_t* out_disassembly_bytes,
	char** out_disassembly
) noexcept
{
	if (spirv_bytes < sizeof(spirv_header))
		return spvcpu::result::shader_too_small;

	const uint32_t* shader_words = static_cast<const uint32_t*>(spirv);

	std::unique_ptr<uint32_t, deleter> copied_shader_data;

	if (result header_result = check_header(shader_words); header_result == result::wrong_endianness)
	{
		copied_shader_data = std::unique_ptr<uint32_t, deleter>(static_cast<uint32_t*>(malloc(spirv_bytes)));

		if (copied_shader_data == nullptr)
			return result::no_memory;

		for (uint32_t i = 0; i != spirv_bytes / 4; ++i)
			copied_shader_data.get()[i] = reverse_endianness(shader_words[i]);
		
		shader_words = copied_shader_data.get();

		if (result header_result = check_header(shader_words); header_result != result::success)
			return header_result;
	}
	else if (header_result != result::success)
		return header_result;

	output_buffer output;

	if (result rst = output.initialize(); rst != result::success)
		return rst;

	if (spirv_bytes & 3)
		return result::shader_size_not_divisible_by_four;

	const uint32_t* word_end = shader_words + (spirv_bytes >> 2);

	for(const uint32_t* word = shader_words + 5; word < word_end;)
	{
		uint32_t wordcount = *word >> 16;

		Op opcode = static_cast<Op>(*word & 0xFFFF);

		if (word + wordcount > word_end)
			return result::instruction_past_data_end;

		spird::elem_data op_data;

		if (result rst = spird::get_elem_data(spird, spird::enum_id::Instruction, static_cast<uint32_t>(opcode), &op_data); rst != result::success)
			return rst;

		if (result rst = output.print_instruction_name(op_data.name); rst != result::success)
			return rst;

		const uint32_t* arg_word = word + 1;

		for (uint32_t arg = 0; arg != op_data.argc; ++arg)
			if (result rst = output.print_arg(spird, op_data.arg_types[arg], arg_word, word + wordcount); rst != result::success)
				return rst;

		if (result rst = output.end_line(); rst != result::success)
			return rst;

		if (arg_word != word + wordcount)
			return result::instruction_wordcount_mismatch;

		word = arg_word;
	}

	if (result rst = output.finalize(); rst != result::success)
		return rst;
	
	*out_disassembly_bytes = output.size();

	*out_disassembly = output.steal();

	return result::success;
}
