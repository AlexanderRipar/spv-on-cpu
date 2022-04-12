#ifndef ID_DATA_HPP_INCLUDE_GUARD
#define ID_DATA_HPP_INCLUDE_GUARD

#include <cstdint>

#include "spv_result.hpp"
#include "spird_defs.hpp"
#include "simple_vec.hpp"

// This pragma is necessary to force the union to have its actual required size.
// Without packing, the union takes up 16 bytes, even though only at most 12
// are used. This would mean that the additional discriminant and id would
// push the size of type_data to 24, instead of the 16 it really should be.
#pragma pack(push, 1)
union raw_type_data
{
	// struct void_data_t{} void_data;
		
	// struct bool_data_t{} bool_data;

	struct int_data_t
	{
		uint8_t width;

		bool is_signed;
	} int_data;

	struct float_data_t
	{
		uint8_t width;
	} float_data;

	struct vector_data_t
	{
		spird::arg_type component_type;

		union
		{
			float_data_t float_component;
			
			int_data_t int_component;

			// bool_data_t bool_component;
		};

		uint8_t component_count;
	} vector_data;

	struct matrix_data_t
	{
		vector_data_t column_data;

		uint8_t column_count;
	} matrix_data;

	struct image_data_t
	{
		spird::arg_type sample_type;

		union
		{
			int_data_t sample_int;

			float_data_t sample_float;
		};
			
		uint8_t dim;

		uint8_t depth;
			
		uint8_t arrayed;
			
		uint8_t ms;
			
		uint8_t sampled;
			
		uint8_t format;

		// 0xFF if none is provided.
		uint8_t access_qualifier;
	} image_data;

	// struct sampler_data_t{} sampler_data;

	image_data_t sampled_image_data;

	struct array_data_t
	{
		uint64_t length;

		uint32_t element_id;
	} array_data;

	struct runtime_array_data_t
	{
		uint32_t element_id;
	} runtime_array_data;

	struct struct_data_t
	{
		const uint32_t* elements;

		uint32_t element_count;
	} struct_data;

	struct opaque_data_t
	{
		const char* name;
	} opaque_data;

	struct pointer_data_t
	{
		uint32_t storage_class;

		uint32_t pointee_id;
	} pointer_data;

	struct function_data_t
	{
		const uint32_t* argv_ids;

		uint32_t return_type_id;
			
		uint8_t argc;
	} function_data;

	// struct event_data_t {} event_data;

	// struct device_event_data_t {} device_event_data;

	// struct reserve_id_data_t {} reserve_id_data;

	// struct queue_data_t {} queue_data;

	struct pipe_data_t
	{
		uint8_t access_qualifier;
	} pipe_data;

	// struct pipe_storage_data_t {} pipe_storage_data;

	// struct named_barrier_data_t {} named_barrier_data;

	struct buffer_surface_intel_data_t
	{
		uint8_t access_qualifier;
	} buffer_surface_intel_data;

	// struct ray_query_khr_data_t {} ray_query_khr_data;

	// struct acceleration_structure_khr_data_t {} acceleration_structure_khr_data;

	struct cooperative_matrix_nv_data_t
	{
		uint32_t scope_id;

		uint32_t rows_id;

		uint32_t columns_id;

		spird::arg_type component_type;

		union
		{
			float_data_t float_component;
			
			int_data_t int_component;

			// bool_data_t bool_component;
		};
	} cooperative_matrix_nv_data;
	
	struct string_data_t
	{
		const char* string;
	} string_data;

	struct ext_inst_set_data_t
	{
		const char* name;
	} ext_inst_set_data;

	struct label_data_t
	{
		const uint32_t* location;
	} label_data;

	// struct deco_group_data_t {} deco_group_data;
};
#pragma pack(pop)

struct alignas(uint64_t) type_data
{
	raw_type_data m_data;
	
	// This goes at the end so it doesn't cause any misalignment within the union
	spird::arg_type m_type;
};

static_assert(sizeof(type_data) == 16);

struct id_type_map
{
private:

	struct id_data_mapper{
		uint32_t id; 
		uint32_t data_index;
	};

	simple_vec<type_data> m_data;
	
	simple_table<id_data_mapper> m_ids;

	uint32_t m_used_ids;

	uint8_t m_table_size_log2;

	bool add_internal(id_data_mapper mapper) noexcept
	{
		uint32_t size = m_ids.size();

		if ((m_used_ids * 3) >> 1 > size)
		{
			id_data_mapper* old = m_ids.steal();

			if (!m_ids.initialize(size * 2))
			{
				free(old);

				return false;
			}

			m_ids.memset(0xFF);

			m_table_size_log2 *= 2;

			const uint32_t mask = (1 << m_table_size_log2) - 1;

			for (uint32_t i = 0; i != size; ++i)
			{
				uint32_t h = hash(old[i].id, m_table_size_log2);

				while (m_ids[h].id != ~0u)
					h = (h + 1) & mask;

				m_ids[h] = old[i];
			}

			free(old);
		}

		const uint32_t mask = (1 << m_table_size_log2) - 1;

		uint32_t h = hash(mapper.id, m_table_size_log2);

		while (m_ids[h].id != ~0u)
			h = (h + 1) & mask;

		m_ids[h] = mapper;

		return true;
	}

	static uint32_t hash(uint32_t id, uint32_t table_size_log2) noexcept
	{
		return (id * 2654435769) >> 32 - table_size_log2;
	}

public:

	spvcpu::result initialize() noexcept
	{
		if (!m_data.initialize(512) || !m_ids.initialize(1 << 11))
			return spvcpu::result::no_memory;

		m_ids.memset(0xFF);

		m_table_size_log2 = 11;

		m_used_ids = 0;

		return spvcpu::result::success;
	}

	spvcpu::result add(uint32_t id, uint32_t type_id) noexcept
	{
		uint32_t type_h = hash(type_id, m_table_size_log2);

		const uint32_t initial_type_h = type_h;

		while (m_ids[type_h].id == ~0u)
		{
			type_h = (type_h + 1);

			if (type_h == initial_type_h)
				return spvcpu::result::id_not_found;
		}
		
		if (!add_internal({ id, type_h }))
			return spvcpu::result::no_memory;
		
		return spvcpu::result::success;
	}

	spvcpu::result add(uint32_t id, const type_data& data) noexcept
	{
		if (!m_data.append(data))
			return spvcpu::result::no_memory;

		if (!add_internal({ id, m_data.size() -1 }))
			return spvcpu::result::no_memory;

		return spvcpu::result::success;
	}

	spvcpu::result get(uint32_t id, type_data** out_data) noexcept
	{
		uint32_t h = hash(id, m_table_size_log2);

		const uint32_t mask = (1 << m_table_size_log2) - 1;

		const uint32_t initial_h = h;

		while (m_ids[h].id != id)
		{
			h = (h + 1) & mask;

			if (h == initial_h)
				return spvcpu::result::id_not_found;
		}
		
		*out_data = &m_data[m_ids[h].data_index];

		return spvcpu::result::success;
	}
};

#endif // ID_DATA_HPP_INCLUDE_GUARD
