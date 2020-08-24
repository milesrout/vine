#include <stddef.h>
#include <stdint.h>
#include "types.h"
#include "hash.h"

#ifdef __GNUC__
#define FNV_OFFSET_BASIS_128 U128_LITERAL(0x0000000001000000, 0x000000000000013B)
#define FNV_PRIME_128        U128_LITERAL(0x6c62272e07bb0142, 0x62b821756295c58d)

u64
fnv1a_nt(const u8 *data)
{
	u128 hash = FNV_OFFSET_BASIS_128;

	while (*data) {
		hash = hash ^ *data++;
		hash = hash * FNV_PRIME_128;
	}

	return (u64)(hash >> 64) ^ (u64)hash;
}

u64
fnv1a(const u8 *data, size_t len)
{
	u128 hash = FNV_OFFSET_BASIS_128;
	size_t i = 0;

	for (; i < len; i++) {
		hash = hash ^ data[i];
		hash = hash * FNV_PRIME_128;
	}

	return (u64)(hash >> 64) ^ (u64)hash;
}
#else
#error "128-bit arithmetic is not implemented for your platform"
#endif
