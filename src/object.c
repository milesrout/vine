#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "types.h"
#include "alloc.h"
#include "memory.h"
#include "hash.h"
#include "object.h"
#include "table.h"
#include "abort.h"

const char *
vobject_typename(struct object_vtable **obj)
{
	return (*obj)->typename(obj);
}

u64
vobject_hash(struct object_vtable **obj)
{
	return (*obj)->hash(obj);
}

int
vobject_equal(struct object_vtable **l, struct object_vtable **r)
{
	if (*l != *r && *l != &indirect_object_vtable && *r != &indirect_object_vtable)
		return 0;
	return (*l)->equal(l, r);
}

struct object_vtable **
vobject_copy(struct object_vtable **obj)
{
	return (*obj)->copy(obj);
}

void
vobject_deinit(struct object_vtable **obj)
{
	(*obj)->deinit(obj);
}

void
vobject_destroy(struct object_vtable **obj, struct alloc *alloc)
{
	(*obj)->destroy(obj, alloc);
}

#define DEFINE_VTABLE(name) \
struct name##_object; \
static const \
struct object_vtable \
name##_object_vtable = { \
	&name##_object_typename, \
	&name##_object_hash, \
	&name##_object_equal, \
	&name##_object_copy, \
	&name##_object_deinit, \
	&name##_object_destroy, \
}

#define DEFINE_PRIMITIVE_VOBJECT(name, type) \
static struct object_vtable name##_object_vtable; \
static int name##_object_equal(struct object_vtable **l, struct object_vtable **r); \
static u64 name##_object_hash(struct object_vtable **); \
\
void \
name##_object_init(struct name##_object *obj, type value) \
{ \
	obj->vtable = &name##_object_vtable; \
	obj->value = value; \
} \
struct object_vtable ** \
vobject_from_##name(type value) \
{ \
	struct name##_object *obj = allocate(sizeof *obj); \
	obj->vtable = &name##_object_vtable; \
	obj->value = value; \
	return &obj->vtable; \
} \
int \
vobject_is_##name(struct object_vtable **obj) \
{ \
	return *obj == &name##_object_vtable; \
} \
type \
vobject_as_##name(struct object_vtable **obj) \
{ \
	type value; \
	if (vobject_try_as_##name(obj, &value)) { \
		abort_with_error("object_as_" #name " failed: got %s\n", \
			vobject_typename(obj)); \
	} \
	return value; \
} \
int \
vobject_try_as_##name(struct object_vtable **obj, type *out) \
{ \
	if (!vobject_is_##name(obj)) { \
		return 1; \
	} \
	*out = ((struct name##_object *)obj)->value; \
	return 0; \
} \
static \
const char * \
name##_object_typename(struct object_vtable **obj) \
{ \
	(void)obj; \
	return #name; \
} \
static \
struct object_vtable ** \
name##_object_copy(struct object_vtable **obj) \
{ \
	struct name##_object *o = (struct name##_object *)obj; \
	return vobject_from_##name(o->value); \
} \
static \
void \
name##_object_deinit(struct object_vtable **obj) \
{ \
	(void)obj;\
} \
static \
void \
name##_object_destroy(struct object_vtable **obj, struct alloc *alloc) \
{ \
	struct name##_object *o = (struct name##_object *)obj; \
	name##_object_deinit(obj); \
	deallocate_with(alloc, o, sizeof *o); \
} \
static \
struct object_vtable \
name##_object_vtable = { \
	&name##_object_typename, \
	&name##_object_hash, \
	&name##_object_equal, \
	&name##_object_copy, \
	&name##_object_deinit, \
	&name##_object_destroy, \
}

DEFINE_PRIMITIVE_VOBJECT(u8,  u8);
DEFINE_PRIMITIVE_VOBJECT(u16, u16);
DEFINE_PRIMITIVE_VOBJECT(u32, u32);
DEFINE_PRIMITIVE_VOBJECT(u64, u64);
DEFINE_PRIMITIVE_VOBJECT(z8,  z8);
DEFINE_PRIMITIVE_VOBJECT(z16, z16);
DEFINE_PRIMITIVE_VOBJECT(z32, z32);
DEFINE_PRIMITIVE_VOBJECT(z64, z64);
DEFINE_PRIMITIVE_VOBJECT(int, int);
DEFINE_PRIMITIVE_VOBJECT(cstr, const char *);
/* DEFINE_PRIMITIVE_VOBJECT(indirect, struct object_vtable **); */

#define DEFINE_BUILTIN_EQUALITY_OBJECT(name, type) \
static \
int \
name##_object_equal(struct object_vtable **l, struct object_vtable **r) \
{ \
	struct name##_object *lo = (struct name##_object *)l; \
	struct name##_object *ro = (struct name##_object *)r; \
	return lo->value == ro->value; \
} \
static \
u64 \
name##_object_hash(struct object_vtable **obj) \
{ \
	struct name##_object *o = (struct name##_object *)obj; \
	return fnv1a((u8 *)&o->value, sizeof o->value); \
}

DEFINE_BUILTIN_EQUALITY_OBJECT(u8,  u8)
DEFINE_BUILTIN_EQUALITY_OBJECT(u16, u16)
DEFINE_BUILTIN_EQUALITY_OBJECT(u32, u32)
DEFINE_BUILTIN_EQUALITY_OBJECT(u64, u64)
DEFINE_BUILTIN_EQUALITY_OBJECT(z8,  z8)
DEFINE_BUILTIN_EQUALITY_OBJECT(z16, z16)
DEFINE_BUILTIN_EQUALITY_OBJECT(z32, z32)
DEFINE_BUILTIN_EQUALITY_OBJECT(z64, z64)
DEFINE_BUILTIN_EQUALITY_OBJECT(int, int)

static
int
cstr_object_equal(struct object_vtable **l, struct object_vtable **r)
{
	struct cstr_object *lo = (struct cstr_object *)l;
	struct cstr_object *ro = (struct cstr_object *)r;
	return strcmp(lo->value, ro->value) == 0;
}

static
u64
cstr_object_hash(struct object_vtable **obj)
{
	struct cstr_object *o = (struct cstr_object *)obj;
	return fnv1a_nt((const u8 *)o->value);
}

static
const char *
indirect_object_typename(struct object_vtable **obj)
{
	struct indirect_object *o = (struct indirect_object *)obj;
	return vobject_typename(o->value);
}

static
int
indirect_object_equal(struct object_vtable **l, struct object_vtable **r)
{
	struct indirect_object *lo = (struct indirect_object *)l;
	struct indirect_object *ro = (struct indirect_object *)r;
	return vobject_equal(lo->value, ro->value);
}

static
u64
indirect_object_hash(struct object_vtable **obj)
{
	struct indirect_object *o = (struct indirect_object *)obj;
	return vobject_hash(o->value);
}

/*
static
struct object_vtable **
indirect_object_copy(struct object_vtable **obj)
{
	struct indirect_object *io = (struct indirect_object *)obj;
	return vobject_copy(io->value);
}
*/

static
void
indirect_object_deinit(struct object_vtable **obj)
{
	struct indirect_object *io = (struct indirect_object *)obj;
	vobject_deinit(io->value);
}

static
void
indirect_object_destroy(struct object_vtable **obj, struct alloc *alloc)
{
	struct indirect_object *io = (struct indirect_object *)obj;
	vobject_destroy(io->value, alloc);
}

struct object_vtable
indirect_object_vtable = {
	&indirect_object_typename,
	&indirect_object_hash,
	&indirect_object_equal,
	NULL, /*&indirect_object_copy,*/
	&indirect_object_deinit,
	&indirect_object_destroy,
};

const char *
object_typename(union object o)
{
	return vobject_typename(&o.vtable);
}

u64
object_hash(union object o)
{
	return vobject_hash(&o.vtable);
}

int
object_equal(union object l, union object r)
{
	return vobject_equal(&l.vtable, &r.vtable);
}

union object
object_copy(union object o)
{
	if (o.vtable == &indirect_object_vtable) {
		union object result;
		result.indirect.vtable = &indirect_object_vtable;
		result.indirect.value = vobject_copy(o.indirect.value);
		return result;
	}
	/* union object is a value type other than indirect */
	return o;
}

void
object_deinit(union object o)
{
	vobject_deinit(&o.vtable);
}

void
object_destroy(union object o, struct alloc *alloc)
{
	vobject_destroy(&o.vtable, alloc);
}

#define DEFINE_INLINE_OBJECT(name, type) \
void \
object_init_from_##name(union object *o, type value) \
{ \
	name##_object_init(&o->name##_value, value); \
} \
int \
object_is_##name(union object *o) \
{ \
	return vobject_is_##name(&o->vtable); \
} \
type \
object_as_##name(union object *o) \
{ \
	return vobject_as_##name(&o->vtable); \
} \
int \
object_try_as_##name(union object *o, type *value) \
{ \
	return vobject_try_as_##name(&o->vtable, value); \
} \

DEFINE_INLINE_OBJECT(u8,  u8)
DEFINE_INLINE_OBJECT(u16, u16)
DEFINE_INLINE_OBJECT(u32, u32)
DEFINE_INLINE_OBJECT(u64, u64)
DEFINE_INLINE_OBJECT(z8,  z8)
DEFINE_INLINE_OBJECT(z16, z16)
DEFINE_INLINE_OBJECT(z32, z32)
DEFINE_INLINE_OBJECT(z64, z64)
DEFINE_INLINE_OBJECT(int, int)
DEFINE_INLINE_OBJECT(cstr, const char *)
