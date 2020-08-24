#ifndef VINE_TYPES_H_INCLUDED
#error "Must include types.h before including hash.h"
#endif
#ifdef VINE_HASH_H_INCLUDED
#error "May not include hash.h more than once"
#endif
#define VINE_HASH_H_INCLUDED
extern u64 fnv1a(const u8 *data, size_t len);
extern u64 fnv1a_nt(const u8 *data);
