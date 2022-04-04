
#include <Windows.h>
#include <cstdio>
#include <cstdint>

using asm_sig_64_bin_op = uint64_t (*) (uint64_t, uint64_t);

using asm_sig_ptr64_ld_op = uint64_t (*) (uint64_t*);

int main(int argc, const char** argv)
{
	uint8_t* mem = static_cast<uint8_t*>(VirtualAlloc(nullptr, 1024, MEM_COMMIT, PAGE_READWRITE));

	if (mem == nullptr)
	{
		fprintf(stderr, "VirtualAlloc failed. (%d)\n", GetLastError());

		return 1;
	}

	uint32_t curr = 0;


	// uint64_t (*) (uint64_t*)

	mem[curr++] = 0x48; // REX 64-bit

	mem[curr++] = 0x31; // XOR

	mem[curr++] = 0b11'000'000; // RAX RAX



	mem[curr++] = 0x48; // REX 64-bit

	mem[curr++] = 0x8B; // MOV (RM -> REG)

	mem[curr++] = 0b00'000'001; // [RCX] RAX

	/*
	// uint64_t (*) (uint64_t, uint64_t);

	mem[curr++] = 0x48; // REX 64-bit

	mem[curr++] = 0x01; // ADD

	mem[curr++] = 0b11'010'001; // ... RDX to RCX

	mem[curr++] = 0x48; // REX 64-bit

	mem[curr++] = 0x89; // MOV

	mem[curr++] = 0b11'001'000; // RCX to RAX
	*/

	/*
	// uint32_t (*) (uint32_t, uint32_t)

	mem[curr++] = 0x31; // XOR

	mem[curr++] = 0b11'000'000; // EAX EAX
	*/

	mem[curr++] = 0xC3; // RETN

	DWORD old_prot;

	if (VirtualProtect(mem, 1024, PAGE_EXECUTE_READ, &old_prot) == 0)
	{
		fprintf(stderr, "VirtualProtect failed. (%d)\n", GetLastError());

		return 1;
	}

	auto asm_function = reinterpret_cast<asm_sig_ptr64_ld_op>(mem);

	uint64_t val = 0x1234'5678'90FFui64;

	uint64_t result = asm_function(&val);

	printf("Result of %llu (0x%llX) == %llu (0x%llX)\n", val, val, result, result);

	return 0;
}
