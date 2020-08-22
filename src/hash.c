#include <stddef.h>
#include <stdint.h>
#include "types.h"
#include "hash.h"

#define FNV_OFFSET_BASIS 0xcbf29ce484222325
#define FNV_PRIME        0x00000100000001b3

u64
fnv1a_nt(const u8 *data)
{
	u64 hash = FNV_OFFSET_BASIS;

	while (*data) {
		hash = hash ^ *data++;
		hash = hash * FNV_PRIME;
	}

	return hash;
}

u64
fnv1a(const u8 *data, size_t len)
{
	u64 hash = FNV_OFFSET_BASIS;
	size_t i = 0;

	for (; i < len; i++) {
		hash = hash ^ data[i];
		hash = hash * FNV_PRIME;
	}

	return hash;
}

u32
fnv1a32_nt(const u8 *data)
{
	u64 hash = fnv1a_nt(data);
	return (u32)(hash >> 32) ^ (u32)hash;
}

u32
fnv1a32(const u8 *data, size_t len)
{
	u64 hash = fnv1a(data, len);
	return (u32)(hash >> 32) ^ (u32)hash;
}
