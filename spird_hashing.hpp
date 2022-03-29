#ifndef SPV_DATA_HASHING_HPP_INCLUDE_GUARD
#define SPV_DATA_HASHING_HPP_INCLUDE_GUARD

#include <cstdint>

uint32_t hash_knuth(uint32_t v, uint32_t table_size) noexcept;

#endif // SPV_DATA_HASHING_HPP_INCLUDE_GUARD