
#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

#define UWOP_PUSH_NONVOL     0
#define UWOP_ALLOC_LARGE     1
#define UWOP_ALLOC_SMALL     2
#define UWOP_SET_FPREG       3
#define UWOP_SAVE_NONVOL     4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128     8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME  10

#define REGIND_AX  0
#define REGIND_CX  1
#define REGIND_DX  2
#define REGIND_BX  3
#define REGIND_SP  4
#define REGIND_BP  5
#define REGIND_SI  6
#define REGIND_DI  7
#define REGIND_R8  8
#define REGIND_R9  9
#define REGIND_R10 10
#define REGIND_R11 11
#define REGIND_R12 12
#define REGIND_R13 13
#define REGIND_R14 14
#define REGIND_R15 15

struct unwind_code
{
	uint8_t prolog_offset;
	uint8_t op_code : 4;
	uint8_t op_info : 4;
};

static_assert(sizeof(unwind_code) == 2);

struct unwind_info
{
	uint8_t version : 3;
	uint8_t flags : 5;
	uint8_t prolog_size;
	uint8_t unwind_code_cnt;
	uint8_t frame_register : 4;
	uint8_t frame_offset : 4;
	// unwind_code unwind_data[];
};

static_assert(sizeof(unwind_info) == 4);

static uint64_t s_exception_rip_address = 0;

DWORD allocate_dynamic_code_range(size_t code_bytes, void** out_code_addr, bool custom_exception_handlers) noexcept
{
	SYSTEM_INFO sys_info;

	GetSystemInfo(&sys_info);

	const uint32_t page_size = sys_info.dwPageSize;

	const size_t total_alloc_size = code_bytes + page_size + (custom_exception_handlers ? page_size : 0);

	void* mem = VirtualAlloc(nullptr, total_alloc_size, MEM_COMMIT, PAGE_READWRITE);

	if (mem == nullptr)
		return GetLastError();

	*out_code_addr = static_cast<uint8_t*>(mem) + page_size;

	return 0;
}

DWORD finalize_dynamic_code_range(
	size_t code_bytes,
	void* code_addr,
	uint32_t func_cnt,
	const void* const * func_begs,
	const void* const * func_ends,
	const uint32_t* prolog_bytes,
	const uint32_t* unwind_code_cnts,
	const unwind_code* const * unwind_codes,
	uint32_t exception_handler_cnt,
	const void** exception_handlers,
	const uint32_t* func_exception_handler_indices) noexcept
{
	// Just to know the page size
	SYSTEM_INFO sys_info;

	GetSystemInfo(&sys_info);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Check that we have enough space for all our bookkeeping in the single page we allocated for it //
	////////////////////////////////////////////////////////////////////////////////////////////////////

	size_t total_unwind_code_cnt = 0;

	for (uint32_t i = 0; i != func_cnt; ++i)
		total_unwind_code_cnt += unwind_code_cnts[i] + (unwind_code_cnts[i] & 1);

	uint32_t total_handled_func_cnt = 0;

	if (exception_handler_cnt != 0)
		for (uint32_t i = 0; i != func_cnt; ++i)
			total_handled_func_cnt += func_exception_handler_indices[i] != ~0u;

	const size_t total_bookkeeping_bytes = 
		total_unwind_code_cnt * sizeof(unwind_code) + 
		func_cnt * sizeof(RUNTIME_FUNCTION) + 
		func_cnt * sizeof(unwind_info) + 
		total_handled_func_cnt * sizeof(uint32_t);

	if (total_bookkeeping_bytes > sys_info.dwPageSize)
		return ~0u;

	////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////// Create trampolines //////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////

	size_t trampoline_offset_from_code = 0;

	while(code_bytes > trampoline_offset_from_code)
		trampoline_offset_from_code += sys_info.dwPageSize;

	uint8_t* curr_trampoline = static_cast<uint8_t*>(code_addr) + trampoline_offset_from_code;

	const uint8_t* const trampoline_beg = curr_trampoline;

	for (uint32_t i = 0; i != exception_handler_cnt; ++i)
	{
		// movabs RAX, <handler_address>
		// jmp    RAX

		*curr_trampoline++ = 0x48; // REX 64-bit

		*curr_trampoline++ = 0xB8 + REGIND_AX; // mov RAX <- imm64

		memcpy(curr_trampoline, &exception_handlers[func_exception_handler_indices[i]], 8);

		curr_trampoline += 8;

		*curr_trampoline++ = 0xFF; // JMP rm64

		*curr_trampoline++ = 0b11'100'000 | REGIND_AX; // mod = Register immediate, reg = extended opcode 4 (0b100), rm = RAX
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////// Create exception unrolling information //////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	uint8_t* const segment_base = static_cast<uint8_t*>(code_addr) - sys_info.dwPageSize;

	RUNTIME_FUNCTION* func_tbl = reinterpret_cast<RUNTIME_FUNCTION*>(static_cast<uint8_t*>(code_addr) - sys_info.dwPageSize);

	unwind_info* curr_unwind_info = reinterpret_cast<unwind_info*>(func_tbl + func_cnt);

	for (uint32_t i = 0; i != func_cnt; ++i)
	{
		func_tbl[i].BeginAddress = static_cast<const uint8_t*>(func_begs[i]) - segment_base;

		func_tbl[i].EndAddress = static_cast<const uint8_t*>(func_ends[i]) - segment_base;

		func_tbl[i].UnwindInfoAddress = reinterpret_cast<const uint8_t*>(curr_unwind_info) - segment_base;

		curr_unwind_info->version = 1;
		curr_unwind_info->flags = exception_handler_cnt == 0 || func_exception_handler_indices[i] == ~0u ? UNW_FLAG_NHANDLER : UNW_FLAG_EHANDLER;
		curr_unwind_info->prolog_size = prolog_bytes[i];
		curr_unwind_info->unwind_code_cnt = unwind_code_cnts[i];
		curr_unwind_info->frame_register = 0;
		curr_unwind_info->frame_offset = 0;

		memcpy(curr_unwind_info + 1, unwind_codes[i], unwind_code_cnts[i] * sizeof(unwind_code));

		const uint32_t unwind_code_array_size_in_unwind_infos = (unwind_code_cnts[i] + (unwind_code_cnts[i] & 1)) / (sizeof(unwind_info) / sizeof(unwind_code));

		curr_unwind_info += 1 + unwind_code_array_size_in_unwind_infos;

		// Add the exception handler trampoline's segment address onto the end of the unwind_info if we have a handler for the function

		if (exception_handler_cnt != 0 && func_exception_handler_indices[i] != ~0u)
		{
			*reinterpret_cast<uint32_t*>(curr_unwind_info) = trampoline_beg - segment_base + 12 * func_exception_handler_indices[i];

			++curr_unwind_info;

			// Just to make sure that our increment actually works as expected
			static_assert(sizeof(*curr_unwind_info) == sizeof(uint32_t));
		}
	}
	
	if (!RtlAddFunctionTable(func_tbl, func_cnt, reinterpret_cast<uint64_t>(segment_base)))
		return GetLastError();

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Change protection to be exec-only for code and trampolines, and read-only for unwinding data ///
	////////////////////////////////////////////////////////////////////////////////////////////////////

	DWORD old_prot;

	if (!VirtualProtect(code_addr, code_bytes + (exception_handler_cnt == 0 ? 0 : sys_info.dwPageSize), PAGE_EXECUTE, &old_prot))
		return GetLastError();

	if (!VirtualProtect(segment_base, 1, PAGE_READONLY, &old_prot))
		return GetLastError();

	return 0;
}

DWORD free_dynamic_code_range(size_t code_bytes, void* code_addr, uint32_t func_cnt) noexcept
{
	SYSTEM_INFO sys_info;

	GetSystemInfo(&sys_info);

	RUNTIME_FUNCTION* func_tbl = reinterpret_cast<RUNTIME_FUNCTION*>(static_cast<uint8_t*>(code_addr) - sys_info.dwPageSize);

	DWORD err = 0;

	if (!RtlDeleteFunctionTable(func_tbl))
		err = GetLastError();

	if (!VirtualFree(func_tbl, 0, MEM_RELEASE))
		return GetLastError();

	return err;
}

EXCEPTION_DISPOSITION test_exception_handler(PEXCEPTION_RECORD exception_record, ULONG64 establisher_frame, PCONTEXT context_record, PDISPATCHER_CONTEXT dispatcher_context) noexcept
{
	fprintf(stderr, "Caught ze heisse Kartoffel, ja!\n");

	// context_record->Rip += 3;

	return EXCEPTION_DISPOSITION::ExceptionContinueSearch;
}

LONG WINAPI test_top_level_unhandled_exception_filter(PEXCEPTION_POINTERS data)
{
	fprintf(stderr, "Potato incoming!\n");

	if (data->ContextRecord->Rip == s_exception_rip_address)
	{
		data->ContextRecord->Rip += 3;

		fprintf(stderr, "Caught ze hot Kartoscke from ze Distanz!\n");

		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

uint8_t* write_asm(void* code, bool create_exception)
{
	uint8_t* curr = static_cast<uint8_t*>(code);

	*curr++ = 0x41; // REX, B set (rm += 8)

	*curr++ = 0x54; // PUSH R12 (RSP + 8)

	*curr++ = 0x48; // REX 64-bit

	*curr++ = 0x83; // SUB rm <- imm8

	*curr++ = 0b11'101'000 | REGIND_SP; // mod = Register immediate, reg = extended opcode 5 (101b), rm = RSP

	*curr++ = 0x10; // imm8 16

	// END OF PROLOG

	*curr++ = 0x48; // REX 64-bit

	*curr++ = 0x01; // ADD rm <- reg

	*curr++ = 0b11'010'001; // mod = Register immediate, reg = RDX, rm = RCX

	*curr++ = 0x48; // REX 64-bit

	*curr++ = 0x89; // MOV rm <- reg

	*curr++ = 0b11'001'000; // mod = Register immediate, reg = RCX rm = RAX (Return register)

	if (create_exception)
	{
		*curr++ = 0x48; // REX 64-bit

		*curr++ = 0x31; // XOR rm <- reg

		*curr++ = 0b11'001'001; // mod = Register immediate, reg = RCX, rm = RCX

		s_exception_rip_address = reinterpret_cast<uint64_t>(curr);

		*curr++ = 0x48; // REX 64-bit

		*curr++ = 0x8B; // MOV reg <- rm

		*curr++ = 0b00'001'001; // mod = Register indirect, reg = RCX, rm = RCX
	}
	// BEGIN OF EPILOG

	*curr++ = 0x48; // REX 64-bit

	*curr++ = 0x83; // ADD rm <- imm8

	*curr++ = 0b11'000'000 | REGIND_SP; // mod = Register immediate, reg = extended opcode 0 (000b), rm = RSP

	*curr++ = 0x10; // imm8 16

	*curr++ = 0x41; // REX, B set (rm += 8)

	*curr++ = 0x5C; // POP R12 (RSP + 8)

	*curr++ = 0xC3; // RETN

	return curr;
}

// unwind_info: 01 06 02 00 unwind_codes: 06 12     02 70
/* General binary region (in .rdata), exact address is ...E328
  000000014000E2A0: 40 00 00 00 00 00 00 00 01 04 01 00 04 22 00 00  @............"..
  000000014000E2B0: 01 00 00 00 00 00 00 00 01 00 00 00 01 08 01 00  ................
  000000014000E2C0: 08 42 00 00 01 11 01 00 11 62 00 00 01 04 01 00  .B.......b......
  000000014000E2D0: 04 42 00 00 01 09 01 00 09 62 00 00 01 09 01 00  .B.......b......
  000000014000E2E0: 09 E2 00 00 01 09 01 00 09 E2 00 00 01 09 01 00  .â.......â......
  000000014000E2F0: 09 42 00 00 01 30 02 00 17 F2 10 70 01 35 03 00  .B...0...ò.p.5..
  000000014000E300: 1C 01 20 00 15 70 00 00 01 30 02 00 17 F2 10 70  .. ..p...0...ò.p
  000000014000E310: 01 19 02 00 19 32 15 70 01 0A 02 00 0A 32 06 70  .....2.p.....2.p
                                            V unwind_info starts here
  000000014000E320: 01 0E 02 00 0E 12 0A 70 01 06 02 00 06 12 02 70  .......p.......p
  000000014000E330: 19 3D 03 00 12 01 12 00 0B 70 00 00 F0 10 00 00  .=.......p..ð...
  000000014000E340: 80 00 00 00 01 29 03 00 11 01 4E 00 0A 70 00 00  .....)....N..p..
  000000014000E350: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  000000014000E360: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  000000014000E370: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................

  RUNTIME_FUNCTION: 00001F70 00001F86 0000E328

  0000000140001F30: 24 48 8B 04 24 C6 00 09 48 8B 04 24 48 FF C0 48  $H..$Æ..H..$HÿÀH
  0000000140001F40: 89 04 24 48 8B 04 24 C6 00 C3 48 8B 04 24 48 FF  ..$H..$Æ.ÃH..$Hÿ
  0000000140001F50: C0 48 89 04 24 48 8B 04 24 48 83 C4 10 5F C3 CC  ÀH..$H..$H.Ä._ÃÌ
  0000000140001F60: CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC  ÌÌÌÌÌÌÌÌÌÌÌÌÌÌÌÌ
                    V Function starts here
  0000000140001F70: 40 57 48 83 EC 10 8B 04 25 00 00 00 00 89 04 24  @WH.ì...%......$
  0000000140001F80: 48 83 C4 10 5F C3 CC CC CC CC CC CC CC CC CC CC  H.Ä._ÃÌÌÌÌÌÌÌÌÌÌ
                                      ^ Function ends here
  0000000140001F90: 48 89 54 24 10 48 89 4C 24 08 57 48 81 EC 90 00  H.T$.H.L$.WH.ì..
  0000000140001FA0: 00 00 48 8D 7C 24 20 B9 1C 00 00 00 B8 CC CC CC  ..H.|$ ¹....¸ÌÌÌ
  0000000140001FB0: CC F3 AB 48 8B 8C 24 A0 00 00 00 48 8B 05 66 D0  Ìó«H..$....H..fÐ
  0000000140001FC0: 00 00 48 33 C4 48 89 84 24 80 00 00 00 C7 44 24  ..H3ÄH..$....ÇD$
  0000000140001FD0: 74 00 00 00 00 EB 0A 8B 44 24 74 FF C0 89 44 24  t....ë..D$tÿÀ.D$
  0000000140001FE0: 74 8B 44 24 74 48 83 F8 40 74 21 8B 44 24 74 0F  t.D$tH.ø@t!.D$t.
  0000000140001FF0: B6 44 04 30 48 8B 8C 24 A0 00 00 00 48 03 C8 48  ¶D.0H..$....H.ÈH
  0000000140002000: 8B C1 48 89 84 24 A0 00 00 00 EB CB 48 8B 84 24  .ÁH..$....ëËH..$
  0000000140002010: A8 00 00 00 48 8B 8C 24 A0 00 00 00 48 03 C8 48  ¨...H..$....H.ÈH
  0000000140002020: 8B C1 48 8B F8 48 8B CC 48 8D 15 61 9F 00 00 E8  .ÁH.øH.ÌH..a...è
  0000000140002030: CF F1 FF FF 48 8B C7 48 8B 8C 24 80 00 00 00 48  ÏñÿÿH.ÇH..$....H
  0000000140002040: 33 CC E8 E5 F0 FF FF 48 81 C4 90 00 00 00 5F C3  3ÌèåðÿÿH.Ä...._Ã
*/
void __declspec(noinline) die_from_nullptr()
{
	uint32_t x = *static_cast<uint32_t*>(nullptr);
}

uint64_t test_func(uint64_t a, uint64_t b) noexcept
{
	uint8_t inc[64];

	for (uint32_t i = 0; i != sizeof(inc); ++i)
		a += inc[i];

	return a + b;
}

int main(int argc, const char** argv)
{
	bool raise_exception = false;

	bool except_dont_handle = false;

	bool except_handle = false;

	if (argc == 2)
	{
		const char* create_exception_string = "--except";

		const char* except_dont_handle_string = "--unhandled";

		if (strcmp(argv[1], create_exception_string) == 0)
		{
			raise_exception = true;

			except_handle = true;
		}
		else if (strcmp(argv[1], except_dont_handle_string) == 0)
		{
			raise_exception = true;

			except_dont_handle = true;
		}
		else
		{
			printf("Usage: %s [%s|%s]\n", argv[0], create_exception_string, except_dont_handle_string);

			return 0;
		}
	}

	void* code;

	DWORD rst = allocate_dynamic_code_range(1024, &code, except_handle);

	if (rst != 0)
	{
		fprintf(stderr, "allocate_dynamic_code_range failed: %d\n", rst);

		return 1;
	}

	uint64_t base = 0;

	RUNTIME_FUNCTION* func = RtlLookupFunctionEntry(reinterpret_cast<uint64_t>(code), &base, nullptr);

	printf("Before finalizing: base = %llu, begin = %d, end = %d\n", base, func == nullptr ? 0 : func->BeginAddress, func == nullptr ? 0 : func->EndAddress);

	unwind_code codes[2];

	codes[0].op_code = UWOP_ALLOC_SMALL;
	codes[0].op_info = 1; // 1 * 8 + 8 = 16 bytes
	codes[0].prolog_offset = 6;
	codes[1].op_code = UWOP_PUSH_NONVOL;
	codes[1].op_info = REGIND_R12;
	codes[1].prolog_offset = 2;

	fprintf(stderr, "Exception records are %X %X, %X %X\n", reinterpret_cast<uint8_t*>(codes)[0], reinterpret_cast<uint8_t*>(codes)[1], reinterpret_cast<uint8_t*>(codes)[2], reinterpret_cast<uint8_t*>(codes)[3]);

	uint32_t unwind_entry_cnt = 2;

	uint32_t prolog_bytes = 6;

	unwind_code* unwind_codes = codes;

	const void* func_end = write_asm(code, raise_exception);

	const uint32_t exception_handler_cnt = except_handle ? 1 : 0;

	const void* exception_handlers = test_exception_handler;

	const uint32_t func_exception_handler_indices = 0;

	rst = finalize_dynamic_code_range(1024, code, 1, &code, &func_end, &prolog_bytes, &unwind_entry_cnt, &unwind_codes, exception_handler_cnt, &exception_handlers, &func_exception_handler_indices);

	if (rst != 0)
	{
		fprintf(stderr, "finalize_dynamic_code_range failed: %d\n", rst);

		return 1;
	}

	func = RtlLookupFunctionEntry(reinterpret_cast<uint64_t>(code), &base, nullptr);

	if (func != nullptr && except_handle)
	{
		SYSTEM_INFO sys_info;

		GetSystemInfo(&sys_info);

		const uint8_t* segment_base = static_cast<const uint8_t*>(code) - sys_info.dwPageSize;

		const RUNTIME_FUNCTION* func_tbl = reinterpret_cast<const RUNTIME_FUNCTION*>(segment_base);

		const unwind_info* unw_info = reinterpret_cast<const unwind_info*>(segment_base + func_tbl->UnwindInfoAddress);

		const uint32_t trampoline_offset = *(reinterpret_cast<const uint32_t*>(unw_info + 1) + (unw_info->unwind_code_cnt + 1) / 2);

		const void* trampoline_target;

		memcpy(&trampoline_target, segment_base + trampoline_offset + 2, 8);

		printf("Trampoline at %u. Jumps to %p. Expected handler at %p\n", trampoline_offset, trampoline_target, test_exception_handler);
	}

	printf("After finalizing: base = %llu, begin = %d, end = %d\n", base, func == nullptr ? 0 : func->BeginAddress, func == nullptr ? 0 : func->EndAddress);

	uint64_t asm_arg1 = 1, asm_arg2 = 2;

	SetUnhandledExceptionFilter(test_top_level_unhandled_exception_filter);

	uint64_t asm_rst = reinterpret_cast<uint64_t (*) (uint64_t, uint64_t)>(code)(asm_arg1, asm_arg2);

	printf("asm return value: %llu (expected: %llu)\n", asm_rst, asm_arg1 + asm_arg2);

	rst = free_dynamic_code_range(1024, code, 1);

	if (rst != 0)
	{
		fprintf(stderr, "free_dynamic_code_range failed: %d\n", rst);

		return 1;
	}

	func = RtlLookupFunctionEntry(reinterpret_cast<uint64_t>(code), &base, nullptr);

	printf("After freeing: base = %llu, begin = %d, end = %d\n", base, func == nullptr ? 0 : func->BeginAddress, func == nullptr ? 0 : func->EndAddress);

	/*
	// uint64_t (*) (uint64_t*)

	mem[curr++] = 0x48; // REX 64-bit

	mem[curr++] = 0x31; // XOR

	mem[curr++] = 0b11'000'000; // RAX RAX



	mem[curr++] = 0x48; // REX 64-bit

	mem[curr++] = 0x8B; // MOV (RM -> REG)

	mem[curr++] = 0b00'000'001; // [RCX] RAX
	*/
	
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

	// mem[curr++] = 0xC3; // RETN

	return 0;
}