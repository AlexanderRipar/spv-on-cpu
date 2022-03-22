#ifndef SPV_HELPERS_INCLUDE_GUARD
#define SPV_HELPERS_INCLUDE_GUARD

#include "spv_defs.hpp"

#include <cstdint>

struct spirv_header
{
	uint32_t magic;
	uint32_t version;
	uint32_t generator_magic;
	uint32_t id_bound;
	uint32_t reserved_zero;
};

enum class argument_type : uint8_t
{
	none,
	literal,
	string,
	id,
	builtin,
	func_param_attr,
	fp_rounding_mode,
	fp_fast_math_mode,
	linkage_type,
	reserved_fp_denorm_mode,
	reserved_fp_operation_mode,
};

struct decoration_info
{
	const char* name;

	argument_type args[2];

	constexpr decoration_info(const char* name) noexcept : name{ name }, args{ argument_type::none, argument_type::none } {}

	constexpr decoration_info(const char* name, argument_type arg0) noexcept : name{ name }, args{ arg0, argument_type::none } {}

	constexpr decoration_info(const char* name, argument_type arg0, argument_type arg1) noexcept : name{ name }, args{ arg0, arg1 } {}
};

struct builtin_info
{
	const char* name;

	constexpr builtin_info(const char* name) noexcept : name{ name } {}
};

struct function_param_attribute_info
{
	const char* name;

	constexpr function_param_attribute_info(const char* name) noexcept : name{ name } {}
};

struct fp_rounding_mode_info
{
	const char* name;

	constexpr fp_rounding_mode_info(const char* name) noexcept : name{ name } {}
};

struct fp_fast_math_mode_info
{
	const char* name;

	constexpr fp_fast_math_mode_info(const char* name) noexcept : name{ name } {}
};

struct linkage_type_info
{
	const char* name;

	constexpr linkage_type_info(const char* name) noexcept : name{ name } {}
};

struct reserved_fp_denorm_mode_info
{
	const char* name;

	constexpr reserved_fp_denorm_mode_info(const char* name) noexcept : name{ name } {}
};

struct reserved_fp_operation_mode_info
{
	const char* name;

	constexpr reserved_fp_operation_mode_info(const char* name) noexcept : name{ name } {}
};

struct source_language_info
{
	const char* name;

	constexpr source_language_info(const char* name) : name{ name } {}
};

decoration_info get_decoration_info(Decoration d) noexcept;

builtin_info get_builtin_info(Builtin b) noexcept;

function_param_attribute_info get_function_param_attribute_info(FunctionParamAttribute a) noexcept;

fp_rounding_mode_info get_fp_rounding_mode_info(FpRoundingMode m) noexcept;

fp_fast_math_mode_info get_fp_fast_math_mode_info(FpFastMathMode m) noexcept;

linkage_type_info get_linkage_type_info(LinkageType t) noexcept;

reserved_fp_denorm_mode_info get_reserved_fp_denorm_mode_info(ReservedFpDenormMode m) noexcept;

reserved_fp_operation_mode_info get_reserved_fp_operation_mode_info(ReservedFpOperationMode m) noexcept;

source_language_info get_source_language_info(SourceLanguage l) noexcept;

#endif // SPV_HELPERS_INCLUDE_GUARD