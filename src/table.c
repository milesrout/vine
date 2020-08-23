#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include "abort.h"
#include "checked.h"
#include "alloc.h"
#include "types.h"
#include "hash.h"
#include "object.h"
#include "table.h"

static const char *table_object_typename(struct object_vtable **);
static u32 table_object_hash(struct object_vtable **);
static int table_object_equal(struct object_vtable **, struct object_vtable **);
static struct object_vtable **table_object_copy(struct object_vtable **o);
static void table_object_deinit(struct object_vtable **o);
static void table_object_destroy(struct object_vtable **o, struct alloc *alloc);

static int tkey_equal(struct tkey, struct tkey);

struct table *
table_create(struct alloc *alloc, size_t initial_size)
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
table_destroy(struct table *table)
{
	table_deinit(table);
	deallocate_with(table->alloc, table, sizeof(struct table));
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

void
table_deinit(struct table *table)
{
	deallocarray_with(table->alloc, table->pairs, table->capacity, sizeof(struct tpair));
}

int
table_equal(struct table *l, struct table *r)
{
	(void)l, (void)r;
	abort_with_error("table equality not yet implemented");
}

static
struct object_vtable
table_object_vtable = {
	&table_object_typename,
	&table_object_hash,
	&table_object_equal,
	&table_object_copy,
	&table_object_deinit,
	&table_object_destroy
};

void
table_object_init(struct table_object *t, struct alloc *alloc, size_t initial_size)
{
	t->vtable = &table_object_vtable;
	table_init(&t->table, alloc, initial_size);
}

static
const char *
table_object_typename(struct object_vtable **obj)
{
	(void)obj;
	return "table";
}

static
u32
table_object_hash(struct object_vtable **obj)
{
	(void)obj;
	abort_with_error("mutable objects like tables are not hashable");
}

static
int
table_object_equal(struct object_vtable **l, struct object_vtable **r)
{
	/* can assume that l is a table_object, but r might be an indirect_object */
	struct table_object *lo = (struct table_object *)l;
	struct table_object *ro;
	if (*r == &table_object_vtable)
		ro = (struct table_object *)r;
	else if (INDIRECTLY_IS(*r, &table_object_vtable))
		ro = (struct table_object *)INDIRECTLY(r); 
	else
		return 0;
	return table_equal(&lo->table, &ro->table);
}

static
struct object_vtable **
table_object_copy(struct object_vtable **o)
{
	(void)o;
	abort_with_error("table copying not yet implemented");
}

static
void
table_object_deinit(struct object_vtable **obj)
{
	struct table_object *o = (struct table_object *)obj;
	table_deinit(&o->table);
}

static
void
table_object_destroy(struct object_vtable **o, struct alloc *alloc)
{
	table_object_deinit(o);
	deallocate_with(alloc, o, sizeof(struct table_object));
}

int
vobject_is_table(struct object_vtable **vtable)
{
	return *vtable == &table_object_vtable || INDIRECTLY_IS(*vtable, &table_object_vtable);
}

struct table *
vobject_as_table(struct object_vtable **vtable)
{
	struct table *table;

	if (vobject_try_as_table(vtable, &table)) {
		abort_with_error("vobject_as_table failed");
	}

	return table;
}

int
vobject_try_as_table(struct object_vtable **vtable, struct table **table)
{
	struct table_object *to;
	if (*vtable == &table_object_vtable)
		to = (struct table_object *)vtable;
	else if (INDIRECTLY_IS(*vtable, &table_object_vtable))
		to = (struct table_object *)INDIRECTLY(vtable); 
	else
		return 1;

	*table = &to->table;
	return 0;
}

void
object_init_as_table(union object *obj, struct alloc *alloc, size_t initial_size)
{
	struct table_object *to = allocate_with(alloc, sizeof(struct table_object));
	table_object_init(to, alloc, initial_size);
	obj->indirect.vtable = &indirect_object_vtable;
	obj->indirect.value = &to->vtable;
}

/*
union object
object_from_table(struct alloc *alloc, size_t initial_size)
{

}
*/

int
object_is_table(union object *o)
{
	return vobject_is_table(&o->vtable);
}

struct table *
object_as_table(union object *o)
{
	return vobject_as_table(&o->vtable);
}

int
object_try_as_table(union object *o, struct table **value)
{
	return vobject_try_as_table(&o->vtable, value);
}

#define DEFINE_SIMPLE_TABLE_GET_SET(typename) \
int \
table_try_get_##typename(struct table *table, struct tkey key, typename *value) \
{ \
	size_t i; \
\
	for (i = 0; i < table->size; i++) { \
		if (tkey_equal(table->pairs[i].key, key)) { \
			return object_try_as_##typename(&table->pairs[i].value, value); \
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
			object_init_from_##typename(&table->pairs[i].value, value); \
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
	} \
	table->size += 1; \
	table->pairs[i].key = key; \
	object_init_from_##typename(&table->pairs[i].value, value); \
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

static
int
tkey_equal(struct tkey k1, struct tkey k2)
{
	return k1.hash == k2.hash && object_equal(k1.key, k2.key);
}

struct tkey
table_key(union object obj)
{
	struct tkey k;
	k.key = obj;
	k.hash = object_hash(obj);
	return k;
}

#define DEFINE_TKEY_BY_VALUE(name) \
struct tkey \
table_key_##name(name value) \
{\
	struct tkey key;\
\
	key.hash = fnv1a32((u8 *)&value, sizeof(name));\
	object_init_from_##name(&key.key, value); \
\
	return key;\
}

DEFINE_TKEY_BY_VALUE(z8)
DEFINE_TKEY_BY_VALUE(z16)
DEFINE_TKEY_BY_VALUE(z32)
DEFINE_TKEY_BY_VALUE(z64)

DEFINE_TKEY_BY_VALUE(u8)
DEFINE_TKEY_BY_VALUE(u16)
DEFINE_TKEY_BY_VALUE(u32)
DEFINE_TKEY_BY_VALUE(u64)

DEFINE_TKEY_BY_VALUE(int)

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
	object_init_from_cstr(&key.key, value);

	return key;
}
