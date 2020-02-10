#include <stddef.h>
#include <stdint.h>
#include "hash.h"

#define FNV_OFFSET_BASIS 0xcbf29ce484222325
#define FNV_PRIME        0x00000100000001b3

uint32_t
fnv1a(const uint8_t *data, size_t len)
{
	uint64_t hash = FNV_OFFSET_BASIS;
	size_t i = 0;

	for (; i < len; i++)
	{
		hash = hash ^ data[i];
		hash = hash * FNV_PRIME;
	}

	return (uint32_t)(hash >> 32) ^ (uint32_t)hash;
}
