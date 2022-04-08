#include "spv_viewer.hpp"

#include <cstdint>
#include <cstdlib>
#include <memory>

#include "spv_defs.hpp"
#include "spird_defs.hpp"
#include "spird_accessor.hpp"
#include "id_data.hpp"

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

	id_type_map m_id_map;

	spird::rst_type m_rst_type;

	bool m_print_type_info;

	bool grow_string(size_t additional) noexcept
	{
		while (m_string_used + additional > m_string_capacity)
		{
			char* tmp = static_cast<char*>(realloc(m_string, m_string_capacity * 2));

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
			char* tmp = static_cast<char*>(realloc(m_line, m_line_capacity * 2));

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

	bool print_char(char c) noexcept
	{
		if (!grow_line(1))
			return false;

		m_line[m_line_used++] = c;

		return true;
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
		if (!m_print_type_info)
		{
			if (!print_char('T'))
				return false;
		}

		if (!print_char('$') || !grow_line(max_u32_chars))
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

	bool print_u64(uint64_t n) noexcept
	{
		if (!grow_line(max_u64_chars))
			return false;
		
		print_integer_to_buffer(m_line, m_line_used, n, false);

		return true;
	}

	bool print_f64(double n) noexcept
	{
		if (!grow_line(350))
			return false;

		int chars_used = snprintf(m_line + m_line_used, 350, "%f", n);

		if (chars_used >= 0 && chars_used < 350)
		{
			m_line_used += chars_used;

			return true;
		}

		if (chars_used < 0)
			return false;

		if (!grow_line(chars_used - 349))
			return false;

		int chars_used_retry = snprintf(m_line + m_line_used, chars_used + 1, "%f", n);

		if (chars_used_retry < 0 || chars_used_retry > chars_used)
			return print_str("[FloatTooBig]");

		m_line_used += chars_used_retry;

		return true;
	}

	spvcpu::result str_print_rst_and_rtype() noexcept
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
			return spvcpu::result::no_memory;

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
		else
		{
			m_string_used += 2;
		}

		return spvcpu::result::success;
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
				{
					spird::arg_flags flags = elem_data.arg_flags[arg], second_flags = spird::arg_flags::none;

					spird::arg_type type = elem_data.arg_types[arg], second_type = spird::arg_type::INSTRUCTION;

					if ((flags & spird::arg_flags::pair) == spird::arg_flags::pair)
					{
						second_flags = elem_data.arg_flags[arg + 1];

						second_type = elem_data.arg_types[arg + 1];

						++arg;
					}

					if (spvcpu::result rst = print_arg(spird, flags, type, second_flags, second_type, tmp_word, word_end); rst != spvcpu::result::success)
						return rst;
				}
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
			{
				spird::arg_flags flags = elem_data.arg_flags[arg], second_flags = spird::arg_flags::none;

				spird::arg_type type = elem_data.arg_types[arg], second_type = spird::arg_type::INSTRUCTION;

				if ((flags & spird::arg_flags::pair) == spird::arg_flags::pair)
				{
					second_flags = elem_data.arg_flags[arg + 1];

					second_type = elem_data.arg_types[arg + 1];

					++arg;
				}

				if (spvcpu::result rst = print_arg(spird, flags, type, second_flags, second_type, tmp_word, word_end); rst != spvcpu::result::success)
					return rst;
			}
		}

		word = tmp_word;

		return spvcpu::result::success;
	}

	spvcpu::result print_type(const type_data* data) noexcept
	{
		switch (data->m_type)
		{
		case spird::rst_type::Void:
		{
			if (!print_str("void"))
				return spvcpu::result::no_memory;
			break;
		}
		case spird::rst_type::Bool:
		{
			if (!print_str("bool"))
				return spvcpu::result::no_memory;
			break;
		}
		case spird::rst_type::Int:
		{
			if (!print_str(data->m_data.int_data.is_signed ? "int" : "uint"))
				return spvcpu::result::no_memory;

			if (data->m_data.int_data.width != 32)
				if (!print_u32(data->m_data.int_data.width))
					return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Float:
		{
			const char* str = "float";

			if (data->m_data.float_data.width == 64)
				str = "double";
			
			if (!print_str(str))
				return spvcpu::result::no_memory;
			
			if (data->m_data.float_data.width != 32 && data->m_data.float_data.width != 64)
				if (!print_u32(data->m_data.float_data.width))
					return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Vector:
		{
			char prefix = '\0';

			uint8_t width = 0xFF;

			if (data->m_data.vector_data.component_type == spird::rst_type::Float)
			{
				if (data->m_data.vector_data.float_component.width == 16)
					prefix = 'h';
				else if (data->m_data.vector_data.float_component.width == 64)
					prefix = 'd';
				else if (data->m_data.vector_data.float_component.width != 32)
					width = data->m_data.vector_data.float_component.width;
			}
			else if (data->m_data.vector_data.component_type == spird::rst_type::Int)
			{
				if (data->m_data.vector_data.int_component.is_signed)
					prefix = 'i';
				else
					prefix = 'u';

				if (data->m_data.vector_data.int_component.width != 32)
					width = data->m_data.vector_data.int_component.width;
			}
			else if (data->m_data.vector_data.component_type == spird::rst_type::Bool)
			{
				prefix = 'b';
			}
			else
			{
				prefix = '?';
			}

			if (prefix != '\0')
				if (!print_char(prefix))
					return spvcpu::result::no_memory;

			if (width != 0xFF)
				if (!print_u32(width))
					return spvcpu::result::no_memory;

			if (!print_str("vec") || !print_u32(data->m_data.vector_data.component_count))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Matrix:
		{
			char prefix = '\0';

			uint8_t width = 0xFF;

			if (data->m_data.matrix_data.column_data.component_type == spird::rst_type::Float)
			{
				if (data->m_data.matrix_data.column_data.float_component.width == 16)
					prefix = 'h';
				else if (data->m_data.matrix_data.column_data.float_component.width == 64)
					prefix = 'd';
				else if (data->m_data.matrix_data.column_data.float_component.width != 32)
					width = data->m_data.matrix_data.column_data.float_component.width != 32;
			}
			else if (data->m_data.matrix_data.column_data.component_type == spird::rst_type::Int)
			{
				if (data->m_data.matrix_data.column_data.int_component.is_signed)
					prefix = 'i';
				else
					prefix = 'u';

				if (data->m_data.matrix_data.column_data.int_component.width != 32)
					width = data->m_data.matrix_data.column_data.int_component.width != 32;
			}
			else if (data->m_data.matrix_data.column_data.component_type == spird::rst_type::Bool)
			{
				prefix = 'b';
			}
			else
			{
				prefix = '?';
			}

			if (prefix != '\0')
				if (!print_char(prefix))
					return spvcpu::result::no_memory;

			if (width != 0xFF)
				if (!print_u32(width))
					return spvcpu::result::no_memory;

			uint8_t rows = data->m_data.matrix_data.column_data.component_count;

			uint8_t cols = data->m_data.matrix_data.column_count;

			if (!print_str("mat") || !print_u32(cols))
					return spvcpu::result::no_memory;

			if (rows != cols)
				if (!print_char('x') || !print_u32(rows))
					return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Image:
		{
			if (!print_str("Image"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Sampler:
		{
			if (!print_str("Sampler"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::SampledImage:
		{
			if (!print_str("SampledImage"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Array:
		{
			type_data* elem_data;

			if (spvcpu::result rst = m_id_map.get(data->m_data.array_data.element_id, &elem_data); rst != spvcpu::result::success)
				return rst;

			if (spvcpu::result rst = print_type(elem_data); rst != spvcpu::result::success)
				return rst;

			if (!print_char('[') || !print_u64(elem_data->m_data.array_data.length) || !print_char(']'))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::RuntimeArray:
		{
			type_data* elem_data;

			if (spvcpu::result rst = m_id_map.get(data->m_data.runtime_array_data.element_id, &elem_data); rst != spvcpu::result::success)
				return rst;

			if (spvcpu::result rst = print_type(elem_data); rst != spvcpu::result::success)
				return rst;

			if (!print_str("[]"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Struct:
		{
			if (!print_str("Struct"))
				return spvcpu::result::success;
			
			break;
		}
		case spird::rst_type::Opaque:
		{
			if (!print_str("Opaque"))
				return spvcpu::result::success;
			
			break;
		}
		case spird::rst_type::Pointer:
		{
			type_data* pointee_data;

			if (spvcpu::result rst = m_id_map.get(data->m_data.pointer_data.pointee_id, &pointee_data); rst != spvcpu::result::success)
				return rst;

			if (spvcpu::result rst = print_type(pointee_data); rst != spvcpu::result::success)
				return rst;

			if (!print_char('*'))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Function:
		{
			type_data* arg_data;

			if (spvcpu::result rst = m_id_map.get(data->m_data.function_data.return_type_id, &arg_data); rst != spvcpu::result::success)
				return rst;

			if (spvcpu::result rst = print_type(arg_data); rst != spvcpu::result::success)
				return rst;

			if (!print_str(" Func("))

			for (uint8_t i = 0; i != data->m_data.function_data.argc; ++i)
			{
				if (spvcpu::result rst = m_id_map.get(data->m_data.function_data.argv_ids[i], &arg_data); rst != spvcpu::result::success)
					return rst;

				if (spvcpu::result rst = print_type(arg_data); rst != spvcpu::result::success)
					return rst;

				if (i != data->m_data.function_data.argc - 1)
				{
					if (!print_str(", "))
						return spvcpu::result::no_memory;
				}
			}

			if (!print_str(")"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Event:
		{
			if (!print_str("Event"))
				return spvcpu::result::no_memory;
			
			break;
		}
		case spird::rst_type::DeviceEvent:
		{
			if (!print_str("DeviceEvent"))
				return spvcpu::result::no_memory;
			
			break;
		}
		case spird::rst_type::ReserveId:
		{
			if (!print_str("ReserveId"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Queue:
		{
			if (!print_str("Queue"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Pipe:
		{
			if (!print_str("Pipe"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::PipeStorage:
		{
			if (!print_str("PipeStorage"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::NamedBarrier:
		{
			if (!print_str("NamedBarrier"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::BufferSurfaceINTEL:
		{
			if (!print_str("BufferSurfaceINTEL"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::RayQueryKHR:
		{
			if (!print_str("RayQueryKHR"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::AccelerationStructureKHR:
		{
			if (!print_str("AccelerationStructureKHR"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::CooperativeMatrixNV:
		{
			if (!print_str("CooperativeMatrixNV"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::String:
		{
			if (!print_str("String \"") || !print_str(data->m_data.string_data.string) || !print_str("\""))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::ExtInstSet:
		{
			if (!print_str("ExtInstSet") || !print_str(data->m_data.ext_inst_set_data.name))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::Label:
		{
			if (!print_str("Label"))
				return spvcpu::result::no_memory;

			break;
		}
		case spird::rst_type::DecoGroup:
		{
			if (!print_str("DecoGroup"))
				return spvcpu::result::no_memory;

			break;
		}
		default:
		{
			return spvcpu::result::unknown_rsttype;
		}
		}

		return spvcpu::result::success;
	}

	spvcpu::result print_literal(const type_data* data, const uint32_t* literal_word, const uint32_t* literal_end) noexcept
	{
		if (literal_end - literal_word < 1)
			return spvcpu::result::instruction_wordcount_mismatch;

		switch (data->m_type)
		{
		case spird::rst_type::Int:
		{
			uint64_t n = *literal_word;

			if (data->m_data.int_data.width == 64)
			{
				if (literal_end - literal_word < 2)
					return spvcpu::result::instruction_wordcount_mismatch;

				n |= static_cast<uint64_t>(literal_word[1]) << 32;
			}

			if (data->m_data.int_data.is_signed)
			{
				if (!print_i64(static_cast<int64_t>(n)))
					return spvcpu::result::no_memory;
			}
			else
			{
				if (!print_u64(n))
					return spvcpu::result::no_memory;
			}

			break;
		}
		case spird::rst_type::Float:
		{
			uint64_t float_bits = *literal_word;

			double n;

			if (data->m_data.float_data.width == 64)
			{
				if (literal_end - literal_word < 2)
					return spvcpu::result::instruction_wordcount_mismatch;

				float_bits |= static_cast<uint64_t>(literal_word[1]) << 32;

				memcpy(&n, &float_bits, 8); 
			}
			else if (data->m_data.float_data.width == 32)
			{
				float f;

				memcpy(&f, &float_bits, 4);

				n = static_cast<double>(f);
			}
			else if (data->m_data.float_data.width == 16)
			{
				uint64_t sign_bit = (float_bits >> 15) << 63;

				float_bits &= ~(1 << 15);

				uint64_t exponent_raw = float_bits >> 10;

				uint64_t mantissa_raw = float_bits & ((1 << 10) - 1);

				uint64_t exponent = (exponent_raw - 15) + 1023;

				uint64_t mantissa = mantissa_raw;

				if (exponent_raw == 0)
				{
					if (mantissa_raw == 0) // +-0
					{
						exponent = 0;
					}
					else // Denorm
					{
						exponent -= 1;
					}
				}
				else if (exponent_raw == 0x1F)
				{
					exponent = 0x7FFFF;

					if (mantissa_raw != 0) // NaN
					{
						// Make sure we don't signal
						mantissa = 0x8'0000'0000'0001;
					}
				}
				
				uint64_t f64_bits = sign_bit | (exponent << 53) | (mantissa << 43);

				memcpy(&n, &f64_bits, 8);
			}
			else
			{
				return spvcpu::result::unhandled_float_width;
			}

			if (!print_f64(n))
				return spvcpu::result::no_memory;

			break;
		}
		default:
		{
			return spvcpu::result::unexpected_literal_type;
		}
		}

		return spvcpu::result::success;
	}

	spvcpu::result extract_id_type(spird::rst_type type, const uint32_t* word, const uint32_t* word_end) noexcept
	{
		type_data data;

		data.m_type = type;

		// Not passing 'Auto' as type is required from the caller
		switch (type)
		{
		case spird::rst_type::Void:
		case spird::rst_type::Bool:
		case spird::rst_type::Sampler:
		case spird::rst_type::Event:
		case spird::rst_type::DeviceEvent:
		case spird::rst_type::ReserveId:
		case spird::rst_type::Queue:
		case spird::rst_type::PipeStorage:
		case spird::rst_type::NamedBarrier:
		case spird::rst_type::RayQueryKHR:
		case spird::rst_type::AccelerationStructureKHR:
		case spird::rst_type::DecoGroup:
		{
			// All these types are not parameterized.
			break;
		}
		case spird::rst_type::Int:
		{
			data.m_data.int_data.width = word[1];

			data.m_data.int_data.is_signed = word[2];

			break;
		}
		case spird::rst_type::Float:
		{
			data.m_data.float_data.width = word[1];

			break;
		}
		case spird::rst_type::Vector:
		{
			uint32_t component_type_id = word[1];

			type_data* component_type;

			if (spvcpu::result rst = m_id_map.get(component_type_id, &component_type); rst != spvcpu::result::success)
				return rst;

			if (component_type->m_type == spird::rst_type::Bool)
			{
				data.m_data.vector_data.component_type = spird::rst_type::Bool;
			}
			else if (component_type->m_type == spird::rst_type::Int)
			{
				data.m_data.vector_data.component_type = spird::rst_type::Int;

				data.m_data.vector_data.int_component = component_type->m_data.int_data;
			}
			else if (component_type->m_type == spird::rst_type::Float)
			{
				data.m_data.vector_data.component_type = spird::rst_type::Float;

				data.m_data.vector_data.float_component = component_type->m_data.float_data;
			}
			else
			{
				return spvcpu::result::incompatible_types;
			}

			data.m_data.vector_data.component_count = word[2];

			break;
		}
		case spird::rst_type::Matrix:
		{
			uint32_t component_type_id = word[1];

			type_data* component_type;

			if (spvcpu::result rst = m_id_map.get(component_type_id, &component_type); rst != spvcpu::result::success)
				return rst;

			if (component_type->m_type == spird::rst_type::Vector)
			{
				data.m_data.matrix_data.column_data = component_type->m_data.vector_data;
			}
			else
			{
				return spvcpu::result::incompatible_types;
			}

			data.m_data.matrix_data.column_count = word[2];
			
			break;
		}
		case spird::rst_type::Image:
		{
			uint32_t component_type_id = word[1];

			type_data* component_type;

			if (spvcpu::result rst = m_id_map.get(component_type_id, &component_type); rst != spvcpu::result::success)
				return rst;

			if (component_type->m_type == spird::rst_type::Void)
			{
				data.m_data.image_data.sample_type = spird::rst_type::Void;
			}
			else if (component_type->m_type == spird::rst_type::Int)
			{
				data.m_data.image_data.sample_type = spird::rst_type::Int;

				data.m_data.image_data.sample_int = component_type->m_data.int_data;
			}
			else if (component_type->m_type == spird::rst_type::Float)
			{
				data.m_data.image_data.sample_type = spird::rst_type::Float;

				data.m_data.image_data.sample_float = component_type->m_data.float_data;
			}
			else
			{
				return spvcpu::result::incompatible_types;
			}

			data.m_data.image_data.dim = static_cast<uint8_t>(word[2]);

			data.m_data.image_data.depth = static_cast<uint8_t>(word[3]);

			data.m_data.image_data.arrayed = static_cast<uint8_t>(word[4]);

			data.m_data.image_data.ms = static_cast<uint8_t>(word[5]);

			data.m_data.image_data.sampled = static_cast<uint8_t>(word[6]);

			data.m_data.image_data.format = static_cast<uint8_t>(word[7]);
			
			if (word_end - word > 8)
				data.m_data.image_data.access_qualifier = static_cast<uint8_t>(word[8]);
			else
				data.m_data.image_data.access_qualifier = 0xFF;

			break;
		}
		case spird::rst_type::SampledImage:
		{
			uint32_t component_type_id = word[1];

			type_data* component_type;

			if (spvcpu::result rst = m_id_map.get(component_type_id, &component_type); rst != spvcpu::result::success)
				return rst;

			if (component_type->m_type == spird::rst_type::Image)
				data.m_data.sampled_image_data = component_type->m_data.image_data;
			else
				return spvcpu::result::incompatible_types;
			
			break;
		}
		case spird::rst_type::Array:
		{
			data.m_data.array_data.element_id = word[1];

			uint32_t length_id = word[2];

			data.m_data.array_data.length = 0;

			break;
		}
		case spird::rst_type::RuntimeArray:
		{
			data.m_data.runtime_array_data.element_id = word[1];

			break;
		}
		case spird::rst_type::Struct:
		{
			const ptrdiff_t member_cnt = word_end - word - 1;

			data.m_data.struct_data.element_count = member_cnt;

			if (member_cnt != 0)
				data.m_data.struct_data.elements = word + 1;
			else
				data.m_data.struct_data.elements = nullptr;

			break;
		}
		case spird::rst_type::Opaque:
		{
			data.m_data.opaque_data.name = reinterpret_cast<const char*>(word + 1);

			break;
		}
		case spird::rst_type::Pointer:
		{
			data.m_data.pointer_data.storage_class = word[1];

			data.m_data.pointer_data.pointee_id = word[2];

			break;
		}
		case spird::rst_type::Function:
		{
			data.m_data.function_data.return_type_id = word[1];

			data.m_data.function_data.argc = word_end - word - 2;

			data.m_data.function_data.argv_ids = word + 2;

			break;
		}
		case spird::rst_type::Pipe:
		{
			data.m_data.pipe_data.access_qualifier = static_cast<uint8_t>(word[1]);

			break;
		}
		case spird::rst_type::BufferSurfaceINTEL:
		{
			data.m_data.buffer_surface_intel_data.access_qualifier = static_cast<uint8_t>(word[1]);

			break;
		}
		case spird::rst_type::CooperativeMatrixNV:
		{
			uint32_t component_type_id = word[1];

			type_data* component_type;

			if (spvcpu::result rst = m_id_map.get(component_type_id, &component_type); rst != spvcpu::result::success)
				return rst;

			if (component_type->m_type == spird::rst_type::Int)
			{
				data.m_data.cooperative_matrix_nv_data.component_type = spird::rst_type::Int;

				data.m_data.cooperative_matrix_nv_data.int_component = component_type->m_data.int_data;
			}
			else if (component_type->m_type == spird::rst_type::Float)
			{
				data.m_data.cooperative_matrix_nv_data.component_type = spird::rst_type::Float;

				data.m_data.cooperative_matrix_nv_data.float_component = component_type->m_data.float_data;
			}
			else
			{
				return spvcpu::result::incompatible_types;
			}
			
			data.m_data.cooperative_matrix_nv_data.scope_id = word[2];

			data.m_data.cooperative_matrix_nv_data.rows_id = word[3];

			data.m_data.cooperative_matrix_nv_data.columns_id = word[4];

			break;
		}
		case spird::rst_type::String:
		{
			data.m_data.string_data.string = reinterpret_cast<const char*>(word + 1);

			break;
		}
		case spird::rst_type::ExtInstSet:
		{
			data.m_data.ext_inst_set_data.name = reinterpret_cast<const char*>(word + 1);

			break;
		}
		case spird::rst_type::Label:
		{
			data.m_data.label_data.location = word - 1;

			break;
		}
		default:
		{
			return spvcpu::result::unknown_rsttype;
		}
		}

		return m_id_map.add(*word, data);
	}

	spvcpu::result print_single_arg(const void* spird, spird::arg_flags flags, spird::arg_type type, const uint32_t*& word, const uint32_t* word_end) noexcept
	{
		const bool is_id = (flags & spird::arg_flags::id) == spird::arg_flags::id;

		const bool is_result = (flags & spird::arg_flags::result) == spird::arg_flags::result;

		if (!is_result && type != spird::arg_type::RTYPE)
			if (!print_char(' '))
				return spvcpu::result::no_memory;

		if (is_result)
		{
			if (word + 1 > word_end)
				return spvcpu::result::instruction_wordcount_mismatch;

			m_rst_id = *word;

			m_rst_type = static_cast<spird::rst_type>(type);

			if (static_cast<spird::rst_type>(type) != spird::rst_type::Auto)
			{
				if (spvcpu::result rst = extract_id_type(static_cast<spird::rst_type>(type), word, word_end); rst != spvcpu::result::success)
					return rst;
			}
			else
			{
				if (m_rtype_id == ~0u)
					return spvcpu::result::untyped_result;

				if (spvcpu::result rst = m_id_map.add(*word, m_rtype_id); rst != spvcpu::result::success)
					return rst;
			}

			++word;
		}
		else if (is_id)
		{
			if (word + 1 > word_end)
				return spvcpu::result::instruction_wordcount_mismatch;

			if (type == spird::arg_type::RTYPE)
			{
				m_rtype_id = *word;
			}
			else if (type == spird::arg_type::TYPE)
			{
				if (!print_typid(*word))
					return spvcpu::result::no_memory;

				if (m_print_type_info)
				{
					type_data* data;

					if (spvcpu::result rst = m_id_map.get(*word, &data); rst == spvcpu::result::success)
					{
						if (!print_char('('))
							return spvcpu::result::no_memory;

						if (spvcpu::result rst = print_type(data); rst != spvcpu::result::success)
							return rst;

						if (!print_char(')'))
							return spvcpu::result::no_memory;
					}
					else if (rst == spvcpu::result::id_not_found)
					{
						if (!print_str("(?)"))
							return spvcpu::result::no_memory;
					}
					else
					{
						return rst;
					}
				}
			}
			else
			{
				if (!print_id(*word))
					return spvcpu::result::no_memory;
			}

			++word;
		}
		else if (static_cast<uint32_t>(type) < spird::enum_id_count)
		{
			return print_enum(spird, static_cast<spird::enum_id>(type), word, word_end);
		}
		else
		{
			switch (type)
			{
			case spird::arg_type::LITERAL:
			{
				if (m_rtype_id != ~0u)
				{
					type_data* data;

					if (spvcpu::result rst = m_id_map.get(m_rtype_id, &data); rst != spvcpu::result::success)
						return rst;

					if (spvcpu::result rst = print_literal(data, word, word_end); rst != spvcpu::result::success)
						return rst;
				}
				else
				{
					if (!print_str("[LIT ") || !print_u32(word_end - word) || !print_str("]"))
						return spvcpu::result::no_memory;
				}

				word = word_end;

				break;
			}
			case spird::arg_type::RST:
			case spird::arg_type::RTYPE:
			case spird::arg_type::VALUE:
			case spird::arg_type::TYPE:
			case spird::arg_type::UNKNOWN:
			{
				return spvcpu::result::id_arg_without_id;

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

				if (!print_str("\"") || !print_str(str) || !print_str("\""))
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

		return spvcpu::result::success;
	}

public:

	output_buffer() noexcept : m_string{ nullptr }, m_line{ nullptr } {}

	~output_buffer() noexcept
	{
		free(m_string);
	}

	spvcpu::result initialize(bool print_type_info) noexcept
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

		m_print_type_info = print_type_info;

		return m_id_map.initialize();
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

	spvcpu::result print_arg(const void* spird, spird::arg_flags flags, spird::arg_type type, spird::arg_flags second_flag, spird::arg_type second_type, const uint32_t*& word, const uint32_t* word_end) noexcept
	{
		const bool is_optional = (flags & spird::arg_flags::optional) == spird::arg_flags::optional;

		const bool is_variadic = (flags & spird::arg_flags::variadic) == spird::arg_flags::variadic;

		const bool is_pair = (flags & spird::arg_flags::pair) == spird::arg_flags::pair;

		if (is_optional && word == word_end)
			return spvcpu::result::success;

		do
		{
			if (spvcpu::result rst = print_single_arg(spird, flags, type, word, word_end); rst != spvcpu::result::success)
				return rst;

			if (is_pair)
			{
				if (spvcpu::result rst = print_single_arg(spird, second_flag, second_type, word, word_end); rst != spvcpu::result::success)
					return rst;
			}
		}
		while (is_variadic && word < word_end);
		
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
		if (m_rst_id != ~0u)
		{
			spird::rst_type rtype = m_rst_type;

			if (m_rtype_id != ~0u)
			{
				if (spvcpu::result rst = m_id_map.add(m_rst_id, m_rtype_id); rst != spvcpu::result::success)
					return rst;
			}
			else
			{
				switch (m_rst_type)
				{
					
				}
			}
		}

		if (spvcpu::result rst = str_print_rst_and_rtype(); rst != spvcpu::result::success)
			return rst;

		// Don't forget to reserve space for the trailing '\n'
		if (!grow_string(m_line_used + 1))
			return spvcpu::result::no_memory;

		memcpy(m_string + m_string_used, m_line, m_line_used);

		m_string_used += m_line_used + 1;

		m_string[m_string_used - 1] = '\n';

		m_line_used = 0;

		m_rst_id = ~0u;

		m_rtype_id = ~0u;

		m_rst_type = spird::rst_type::Auto;
		
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

__declspec(dllexport) spvcpu::result spvcpu::disassemble(
	uint64_t spirv_bytes,
	const void* spirv,
	const void* spird,
	bool print_type_info,
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

	if (result rst = output.initialize(print_type_info); rst != result::success)
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
		{
			spird::arg_flags flags = op_data.arg_flags[arg], second_flags = spird::arg_flags::none;

			spird::arg_type type = op_data.arg_types[arg], second_type = spird::arg_type::INSTRUCTION;

			if ((flags & spird::arg_flags::pair) == spird::arg_flags::pair)
			{
				second_flags = op_data.arg_flags[arg + 1];

				second_type = op_data.arg_types[arg + 1];

				++arg;
			}

			if (spvcpu::result rst = output.print_arg(spird, flags, type, second_flags, second_type, arg_word, word + wordcount); rst != spvcpu::result::success)
				return rst;
		}

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
