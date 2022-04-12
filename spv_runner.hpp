#include "spv_result.hpp"
#include <cstdint>

namespace spvcpu
{
	struct module_init_info
	{

	};

	struct module_state
	{
		const uint32_t m_variable_count;

		const void* m_variable_data;

		const char** m_variable_names;
	};

	struct cpu_module
	{
		void* m_opaque_data;
	};

	__declspec(dllexport) result create_cpu_module(uint64_t spirv_bytes, const void* spirv, const void* spird, void** out_module) noexcept;

	__declspec(dllexport) result free_cpu_module(void* module) noexcept;

	__declspec(dllexport) result initialize_cpu_module(const void* module, const module_init_info* init_info, module_state* out_initial_state) noexcept;

	__declspec(dllexport) result step_module(const void* initialized_module, module_state* state) noexcept;

	__declspec(dllexport) result free_module_state(module_state* state) noexcept;
}
