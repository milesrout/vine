#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including object.h"
#endif
#ifndef VINE_HASH_H_INCLUDED
#error "Must include hash.h before including object.h"
#endif
#ifdef VINE_OBJECT_H_INCLUDED
#error "May not include object.h more than once"
#endif
#define VINE_OBJECT_H_INCLUDED
struct object_vtable {
	const char *(*typename)(struct object_vtable **);
	u64 (*hash)(struct object_vtable **);
	int (*equal)(struct object_vtable **, struct object_vtable **);
	struct object_vtable **(*copy)(struct object_vtable **);
	void (*deinit)(struct object_vtable **);
	void (*destroy)(struct object_vtable **, struct alloc *);
};
extern const char *vobject_typename(struct object_vtable **);
extern u64 vobject_hash(struct object_vtable **);
extern int vobject_equal(struct object_vtable **, struct object_vtable **);
extern struct object_vtable **vobject_copy(struct object_vtable **);
extern void vobject_deinit(struct object_vtable **);
extern void vobject_destroy(struct object_vtable **, struct alloc *);
#define DECLARE_PRIMITIVE_VOBJECT(name, type)\
struct name##_object {\
	struct object_vtable *vtable;\
	type value;\
};\
extern void name##_object_init(struct name##_object *, type value); \
extern struct object_vtable **vobject_from_##name(type);\
extern int vobject_is_##name(struct object_vtable **);\
extern type vobject_as_##name(struct object_vtable **);\
extern int vobject_try_as_##name(struct object_vtable **, type *)
DECLARE_PRIMITIVE_VOBJECT(u8,  u8);
DECLARE_PRIMITIVE_VOBJECT(u16, u16);
DECLARE_PRIMITIVE_VOBJECT(u32, u32);
DECLARE_PRIMITIVE_VOBJECT(u64, u64);
DECLARE_PRIMITIVE_VOBJECT(z8,  z8);
DECLARE_PRIMITIVE_VOBJECT(z16, z16);
DECLARE_PRIMITIVE_VOBJECT(z32, z32);
DECLARE_PRIMITIVE_VOBJECT(z64, z64);
DECLARE_PRIMITIVE_VOBJECT(int, int);
DECLARE_PRIMITIVE_VOBJECT(cstr, const char *);
DECLARE_PRIMITIVE_VOBJECT(float, float);
DECLARE_PRIMITIVE_VOBJECT(double, double);
struct indirect_object {
	struct object_vtable *vtable;
	struct object_vtable **value;
};
extern struct object_vtable indirect_object_vtable;
#define INDIRECTLY_IS(actual, expected) ( \
	(actual) == &indirect_object_vtable && \
	*((struct indirect_object *)(actual))->value == (expected))
#define INDIRECTLY(vtable) \
	(((struct indirect_object *)(vtable))->value)
union object {
	struct object_vtable  *vtable;
	struct u8_object       u8_value;
	struct u16_object      u16_value;
	struct u32_object      u32_value;
	struct u64_object      u64_value;
	struct z8_object       z8_value;
	struct z16_object      z16_value;
	struct z32_object      z32_value;
	struct z64_object      z64_value;
	struct int_object      int_value;
	struct cstr_object     cstr_value;
	struct float_object    float_value;
	struct double_object   double_value;
	struct indirect_object indirect;
};
#define DECLARE_INLINE_OBJECT(name, type) \
extern void object_init_from_##name(union object *, type); \
extern union object object_from_##name(type); \
extern int object_is_##name(union object *); \
extern type object_as_##name(union object *); \
extern int object_try_as_##name(union object *, type *)
DECLARE_INLINE_OBJECT(u8,  u8);
DECLARE_INLINE_OBJECT(u16, u16);
DECLARE_INLINE_OBJECT(u32, u32);
DECLARE_INLINE_OBJECT(u64, u64);
DECLARE_INLINE_OBJECT(z8,  z8);
DECLARE_INLINE_OBJECT(z16, z16);
DECLARE_INLINE_OBJECT(z32, z32);
DECLARE_INLINE_OBJECT(z64, z64);
DECLARE_INLINE_OBJECT(int, int);
DECLARE_INLINE_OBJECT(cstr, const char *);
DECLARE_INLINE_OBJECT(float, float);
DECLARE_INLINE_OBJECT(double, double);
DECLARE_INLINE_OBJECT(indirect, struct object_vtable **);
extern const char *object_typename(union object);
extern u64 object_hash(union object);
extern int object_equal(union object, union object);
extern union object object_copy(union object);
extern void object_deinit(union object);
extern void object_destroy(union object, struct alloc *);
