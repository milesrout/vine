#include <stddef.h>
#include <stdint.h>
#include "abort.h"
#include "checked.h"
#include "alloc.h"
#include "types.h"
#include "hash.h"
#include "object.h"
#include "table.h"

static
int
tkey_equal(struct tkey k1, struct tkey k2)
{
	return k1.hash == k2.hash && object_equal(k1.obj, k2.obj);
}

struct table *
table_new(struct alloc *alloc, size_t initial_size)
{
	struct table *table = allocate_with(alloc, sizeof(struct table));
	struct tpair *pairs = allocarray_with(alloc,
		sizeof(struct tpair), initial_size);

	table->size = 0;
	table->capacity = initial_size;
	table->pairs = pairs;
	table->alloc = alloc;

	return table;
}

void
table_init(struct table *table, struct alloc *alloc, size_t initial_size)
{
	table->size = 0;
	table->capacity = initial_size;
	table->pairs = allocarray_with(alloc,
		sizeof(struct tpair), initial_size);
	table->alloc = alloc;
}

#define DEFINE_SIMPLE_TABLE_GET_SET(typename) \
int \
table_try_get_##typename(struct table *table, struct tkey key, typename *value) \
{ \
	size_t i; \
\
	for (i = 0; i < table->size; i++) { \
		if (tkey_equal(table->pairs[i].key, key)) { \
			return object_try_as_##typename(table->pairs[i].value, value); \
		} \
	} \
	return -1; \
} \
typename \
table_get_##typename(struct table *table, struct tkey key) \
{ \
	typename value; \
	int res = table_try_get_##typename(table, key, &value); \
	if (res != 0) { \
		abort_with_error("table_get_" #typename " failed: %d\n", res); \
	} \
	return value; \
} \
void \
table_set_##typename(struct table *table, struct tkey key, typename value) \
{ \
	size_t i; \
\
	for (i = 0; i < table->size; i++) { \
		if (tkey_equal(table->pairs[i].key, key)) { \
			table->pairs[i].value = object_from_##typename(value);\
			return; \
		} \
	} \
	if (i == table->capacity) { \
		size_t new_capacity = mul_sz(table->capacity, 2); \
		table->pairs = reallocarray_with(table->alloc, \
			table->pairs, \
			sizeof(struct tpair), \
			table->capacity, \
			new_capacity); \
		table->capacity = new_capacity; \
		table->size += 1; \
		table->pairs[i].key = key; \
		table->pairs[i].value = object_from_##typename(value); \
	}\
\
	return; \
} 

DEFINE_SIMPLE_TABLE_GET_SET(u8)
DEFINE_SIMPLE_TABLE_GET_SET(u16)
DEFINE_SIMPLE_TABLE_GET_SET(u32)
DEFINE_SIMPLE_TABLE_GET_SET(u64)
DEFINE_SIMPLE_TABLE_GET_SET(z8)
DEFINE_SIMPLE_TABLE_GET_SET(z16)
DEFINE_SIMPLE_TABLE_GET_SET(z32)
DEFINE_SIMPLE_TABLE_GET_SET(z64)
DEFINE_SIMPLE_TABLE_GET_SET(int)

#define DEFINE_TKEY_BY_VALUE(name, upname, type) \
struct tkey \
table_key_##name(name value) \
{\
	struct tkey key;\
\
	key.hash = fnv1a32((u8 *)&value, sizeof(name));\
	key.obj = object_from_##name(value); \
	/*key.type = TKEY_##upname;\ key.value.type##value = value;\ */ \
\
	return key;\
}

DEFINE_TKEY_BY_VALUE(z8,  Z8,  z64value)
DEFINE_TKEY_BY_VALUE(z16, Z16, z64value)
DEFINE_TKEY_BY_VALUE(z32, Z32, z64value)
DEFINE_TKEY_BY_VALUE(z64, Z64, z64value)

DEFINE_TKEY_BY_VALUE(u8,  U8,  u64value)
DEFINE_TKEY_BY_VALUE(u16, U16, u64value)
DEFINE_TKEY_BY_VALUE(u32, U32, u64value)
DEFINE_TKEY_BY_VALUE(u64, U64, u64value)

DEFINE_TKEY_BY_VALUE(int, INT, z64value)

#define DEFINE_TKEY_BY_STRUCT_VALUE(name, upname, type) \
struct tkey \
table_key_struct_##name(struct name value) \
{\
	struct tkey key;\
\
	key.hash = fnv1a((u8 *)&value, sizeof(struct name));\
	key.type = TKEY_STRUCT_##upname;\
	key.value.type##value = value;\
\
	return key;\
}

struct tkey
table_key_cstr(const char *value)
{
	struct tkey key;

	key.hash = fnv1a32_nt((const u8 *)value);
	key.obj = object_from_cstr(value);

	return key;
}
