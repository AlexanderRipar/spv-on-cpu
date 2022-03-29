#include "spird_hashing.hpp"

uint32_t hash_knuth(uint32_t v, uint32_t table_size) noexcept
{
	// Use Knuth's hash algorithm

	// We don't really care that we are preserving divisibility. We just want _some_ hashing going on.

	constexpr uint32_t shift = 16;

	constexpr uint32_t knuth = 2654435769;

	uint32_t hash = (v * knuth) >> shift;

	hash = static_cast<uint32_t>((static_cast<uint64_t>(hash) * table_size) >> (32 - shift));

	return hash;
}
