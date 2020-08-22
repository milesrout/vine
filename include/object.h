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
struct object {
	union {
		u64    u64value;
		z64    z64value;
		void  *ptrvalue;
		float  floatvalue;
		double doublevalue;
		char   bytesvalue[14];
	} value;
	u16 type;
};

extern int object_equal(struct object, struct object);

extern struct object object_from_float(float);
extern int object_is_float(struct object);
extern float *object_as_float(struct object);
extern int object_try_as_float(struct object, float *);

extern struct object object_from_double(double);
extern int object_is_double(struct object);
extern double *object_as_double(struct object);
extern int object_try_as_double(struct object, double *);

extern struct object object_from_u8(u8);
extern int object_is_u8(struct object);
extern u8 object_as_u8(struct object);
extern int object_try_as_u8(struct object, u8 *);

extern struct object object_from_u16(u16);
extern int object_is_u16(struct object);
extern u16 object_as_u16(struct object);
extern int object_try_as_u16(struct object, u16 *);

extern struct object object_from_u32(u32);
extern int object_is_u32(struct object);
extern u32 object_as_u32(struct object);
extern int object_try_as_u32(struct object, u32 *);

extern struct object object_from_u64(u64);
extern int object_is_u64(struct object);
extern u64 object_as_u64(struct object);
extern int object_try_as_u64(struct object, u64 *);

extern struct object object_from_z8(z8);
extern int object_is_z8(struct object);
extern z8 object_as_z8(struct object);
extern int object_try_as_z8(struct object, z8 *);

extern struct object object_from_z16(z16);
extern int object_is_z16(struct object);
extern z16 object_as_z16(struct object);
extern int object_try_as_z16(struct object, z16 *);

extern struct object object_from_z32(z32);
extern int object_is_z32(struct object);
extern z32 object_as_z32(struct object);
extern int object_try_as_z32(struct object, z32 *);

extern struct object object_from_z64(z64);
extern int object_is_z64(struct object);
extern z64 object_as_z64(struct object);
extern int object_try_as_z64(struct object, z64 *);

extern struct object object_from_int(int);
extern int object_is_int(struct object);
extern int object_as_int(struct object);
extern int object_try_as_int(struct object, int *);

extern struct object object_from_cstr(const char *);
extern int object_is_cstr(struct object);
extern const char *object_as_cstr(struct object);
extern int object_try_as_cstr(struct object, const char **);

struct table;
extern struct object object_from_table(struct table *);
extern int object_is_table(struct object);
extern struct table *object_as_table(struct object);
extern int object_try_as_table(struct object, struct table **);
