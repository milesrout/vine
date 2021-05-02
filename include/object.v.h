//require alloc.h
//require hash.h
//provide object.h
struct object_vtable {
	const char *(*ovt_typename)(struct object_vtable **);
	u64 (*ovt_hash)(struct object_vtable **);
	int (*ovt_equal)(struct object_vtable **, struct object_vtable **);
	struct object_vtable **(*ovt_copy)(struct object_vtable **);
	void (*ovt_finish)(struct object_vtable **);
	void (*ovt_destroy)(struct object_vtable **, struct alloc *);
};
extern const char *vobject_typename(struct object_vtable **);
extern u64 vobject_hash(struct object_vtable **);
extern int vobject_equal(struct object_vtable **, struct object_vtable **);
extern struct object_vtable **vobject_copy(struct object_vtable **);
extern void vobject_finish(struct object_vtable **);
extern void vobject_destroy(struct object_vtable **, struct alloc *);
#define DECLARE_PRIMITIVE_VOBJECT(name, abbrev, type)\
struct name##_object {\
	struct object_vtable *abbrev##o_vtable;\
	type abbrev##o_value;\
};\
extern void name##_object_init(struct name##_object *, type value); \
extern struct object_vtable **vobject_from_##name(type);\
extern int vobject_is_##name(struct object_vtable **);\
extern type vobject_as_##name(struct object_vtable **);\
extern int vobject_try_as_##name(struct object_vtable **, type *)
DECLARE_PRIMITIVE_VOBJECT(u8,  u8,  u8);
DECLARE_PRIMITIVE_VOBJECT(u16, u16, u16);
DECLARE_PRIMITIVE_VOBJECT(u32, u32, u32);
DECLARE_PRIMITIVE_VOBJECT(u64, u64, u64);
DECLARE_PRIMITIVE_VOBJECT(z8,  z8,  z8);
DECLARE_PRIMITIVE_VOBJECT(z16, z16, z16);
DECLARE_PRIMITIVE_VOBJECT(z32, z32, z32);
DECLARE_PRIMITIVE_VOBJECT(z64, z64, z64);
DECLARE_PRIMITIVE_VOBJECT(int, z, int);
DECLARE_PRIMITIVE_VOBJECT(cstr, sz, const char *);
DECLARE_PRIMITIVE_VOBJECT(float, f, float);
DECLARE_PRIMITIVE_VOBJECT(double, d, double);
struct indirect_object {
	struct object_vtable *io_vtable;
	struct object_vtable **io_value;
};
extern struct object_vtable indirect_object_vtable;
#define INDIRECTLY_IS(actual, expected) ( \
	(actual) == &indirect_object_vtable && \
	*((struct indirect_object *)(actual))->io_value == (expected))
#define INDIRECTLY(vtable) \
	(((struct indirect_object *)(vtable))->io_value)
union object {
	struct object_vtable  *o_vtable;
	struct u8_object       o_u8;
	struct u16_object      o_u16;
	struct u32_object      o_u32;
	struct u64_object      o_u64;
	struct z8_object       o_z8;
	struct z16_object      o_z16;
	struct z32_object      o_z32;
	struct z64_object      o_z64;
	struct int_object      o_int;
	struct cstr_object     o_cstr;
	struct float_object    o_float;
	struct double_object   o_double;
	struct indirect_object o_indirect;
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
extern void object_finish(union object);
extern void object_destroy(union object, struct alloc *);
