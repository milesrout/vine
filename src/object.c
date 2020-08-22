#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "types.h"
#include "alloc.h"
#include "hash.h"
#include "object.h"
#include "abort.h"

enum object_type {
	OBJ_INVALID,
	OBJ_U8,
	OBJ_U16,
	OBJ_U32,
	OBJ_U64,
	OBJ_Z8,
	OBJ_Z16,
	OBJ_Z32,
	OBJ_Z64,
	OBJ_INT,
	OBJ_CSTR,
	OBJ_TABLE,
	OBJ_MAXIMUM_TYPES = 65535
};

int
object_equal(struct object l, struct object r)
{
	if (l.type != r.type) {
		return 0;
	}
	switch (l.type) {
	case OBJ_U8:
	case OBJ_U16:
	case OBJ_U32:
	case OBJ_U64:
		return l.value.u64value == r.value.u64value;
	case OBJ_Z8:
	case OBJ_Z16:
	case OBJ_Z32:
	case OBJ_Z64:
	case OBJ_INT:
		return l.value.z64value == r.value.z64value;
	case OBJ_CSTR:
		return strcmp((char *)l.value.ptrvalue, (char *)r.value.ptrvalue) == 0;
	case OBJ_TABLE: /* compare by identity for now? */
		return l.value.ptrvalue == r.value.ptrvalue;
	default: abort_with_error("invalid object");
	}
}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
struct object
object_from_cstr(const char *value)
{
	struct object obj;
	obj.type = OBJ_CSTR;
	obj.value.ptrvalue = (void *)value;
	return obj;
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

const char *
object_as_cstr(struct object obj)
{
	if (obj.type == OBJ_CSTR)
		return obj.value.ptrvalue;
	else
		abort_with_error("Object not a cstr");
}

int
object_is_cstr(struct object obj)
{
	return obj.type == OBJ_CSTR;
}

#define DEFINE_SIMPLE_OBJECT_TYPE(typename, uppertype, valuetype) \
struct object \
object_from_##typename(typename value) \
{ \
	struct object obj; \
	obj.value.valuetype = value; \
	obj.type = OBJ_##uppertype; \
	return obj; \
} \
int \
object_is_##typename(struct object obj) \
{ \
	return obj.type == OBJ_##uppertype; \
} \
typename \
object_as_##typename(struct object obj) \
{ \
	typename value; \
	int ret = object_try_as_##typename(obj, &value); \
	if (ret != 0) { \
		abort_with_error("object_as_" #typename " failed: %d\n", obj.type); \
	} \
	return value; \
} \
int \
object_try_as_##typename(struct object obj, typename *ptr) \
{ \
	if (obj.type != OBJ_##uppertype) { \
		return obj.type; \
	} \
	*ptr = (typename)obj.value.valuetype; \
	return 0;\
}

DEFINE_SIMPLE_OBJECT_TYPE(u8,  U8,  u64value)
DEFINE_SIMPLE_OBJECT_TYPE(u16, U16, u64value)
DEFINE_SIMPLE_OBJECT_TYPE(u32, U32, u64value)
DEFINE_SIMPLE_OBJECT_TYPE(u64, U64, u64value)
DEFINE_SIMPLE_OBJECT_TYPE(z8,  Z8,  z64value)
DEFINE_SIMPLE_OBJECT_TYPE(z16, Z16, z64value)
DEFINE_SIMPLE_OBJECT_TYPE(z32, Z32, z64value)
DEFINE_SIMPLE_OBJECT_TYPE(z64, Z64, z64value)
DEFINE_SIMPLE_OBJECT_TYPE(int, INT, z64value)


