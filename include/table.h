#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including table.h"
#endif
#ifndef VINE_HASH_H_INCLUDED
#error "Must include hash.h before including table.h"
#endif
#ifndef VINE_OBJECT_H_INCLUDED
#error "Must include object.h before including table.h"
#endif
#ifdef VINE_TABLE_H_INCLUDED
#error "May not include table.h more than once"
#endif
#define VINE_TABLE_H_INCLUDED
struct tkey {
	union object key;
	u32 hash;
};
struct tpair {
	struct tkey key;
	union object value;
};
struct table {
	size_t size, capacity;
	struct tpair *pairs;
	struct alloc *alloc;
};
extern struct table *table_create(struct alloc *, size_t initial_size);
extern void table_deinit(struct table *table);
extern void table_destroy(struct table *table);
extern void table_init(struct table *table, struct alloc *alloc, size_t initial_size);
extern int table_equal(struct table *, struct table *);
/* getting values from a table may fail for two reasons:
 * - the key might be missing
 * - the value might be of a different type
 * in the first case, table_try_get_XXX returns -1
 * in the second case, table_try_get_XXX returns the type that IS there
 */
extern int table_try_get_u8(struct table *, struct tkey, u8 *);
extern int table_try_get_u16(struct table *, struct tkey, u16 *);
extern int table_try_get_u32(struct table *, struct tkey, u32 *);
extern int table_try_get_u64(struct table *, struct tkey, u64 *);
extern int table_try_get_z8(struct table *, struct tkey, z8 *);
extern int table_try_get_z16(struct table *, struct tkey, z16 *);
extern int table_try_get_z32(struct table *, struct tkey, z32 *);
extern int table_try_get_z64(struct table *, struct tkey, z64 *);
extern int table_try_get_int(struct table *, struct tkey, int *);
extern int table_try_get_cstr(struct table *, struct tkey, const char **);
extern u8  table_get_u8(struct table *, struct tkey);
extern u16 table_get_u16(struct table *, struct tkey);
extern u32 table_get_u32(struct table *, struct tkey);
extern u64 table_get_u64(struct table *, struct tkey);
extern z8  table_get_z8(struct table *, struct tkey);
extern z16 table_get_z16(struct table *, struct tkey);
extern z32 table_get_z32(struct table *, struct tkey);
extern z64 table_get_z64(struct table *, struct tkey);
extern int table_get_int(struct table *, struct tkey);
extern const char *table_get_cstr(struct table *, struct tkey);
extern void table_set_u8(struct table *, struct tkey, u8);
extern void table_set_u16(struct table *, struct tkey, u16);
extern void table_set_u32(struct table *, struct tkey, u32);
extern void table_set_u64(struct table *, struct tkey, u64);
extern void table_set_z8(struct table *, struct tkey, z8);
extern void table_set_z16(struct table *, struct tkey, z16);
extern void table_set_z32(struct table *, struct tkey, z32);
extern void table_set_z64(struct table *, struct tkey, z64);
extern void table_set_int(struct table *, struct tkey, int);
extern void table_set_cstr(struct table *, struct tkey, const char *);
extern struct tkey table_key_u8(u8);
extern struct tkey table_key_u16(u16);
extern struct tkey table_key_u32(u32);
extern struct tkey table_key_u64(u64);
extern struct tkey table_key_z8(z8);
extern struct tkey table_key_z16(z16);
extern struct tkey table_key_z32(z32);
extern struct tkey table_key_z64(z64);
extern struct tkey table_key_int(int);
extern struct tkey table_key_cstr(const char *);
extern struct tkey table_key(union object);
struct table_object {
	struct object_vtable *vtable;
	struct table table;
};
extern void table_object_init(struct table_object *, struct alloc *, size_t);
extern void object_init_as_table(union object *, struct alloc *, size_t);
extern union object object_from_table(struct alloc *, size_t);
extern int vobject_is_table(struct object_vtable **);
extern int object_is_table(union object *);
extern struct table *vobject_as_table(struct object_vtable **);
extern struct table *object_as_table(union object *);
extern int vobject_try_as_table(struct object_vtable **, struct table **);
extern int object_try_as_table(union object *, struct table **);
