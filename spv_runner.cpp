#include "spv_runner.hpp"


__declspec(dllexport) spvcpu::result spvcpu::create_cpu_module(uint64_t spirv_bytes, const void* spirv, const void* spird, void** out_module) noexcept
{

}

__declspec(dllexport) spvcpu::result spvcpu::free_cpu_module(void* module) noexcept
{

}

__declspec(dllexport) spvcpu::result spvcpu::initialize_cpu_module(const void* module, const module_init_info* init_info, module_state* out_initial_state) noexcept
{

}

__declspec(dllexport) spvcpu::result spvcpu::step_module(const void* initialized_module, module_state* state) noexcept
{

}

__declspec(dllexport) spvcpu::result spvcpu::free_module_state(module_state* state) noexcept
{

}
