//provide alloc.h
struct alloc;
struct alloc_vtable {
	void *(*avt_allocate)(struct alloc *, size_t);
	void *(*avt_reallocate)(struct alloc *, void *, size_t, size_t);
	void (*avt_deallocate)(struct alloc *, void *, size_t);
};
struct alloc {
	struct alloc_vtable *alloc_vtable;
};
extern struct alloc sys_alloc;
extern struct alloc mmap_alloc;
extern void *allocate_with(struct alloc *, size_t);
extern void *reallocate_with(struct alloc *, void *, size_t, size_t);
extern void *try_allocate_with(struct alloc *, size_t);
extern void *try_reallocate_with(struct alloc *, void *, size_t, size_t);
extern void deallocate_with(struct alloc *, void *, size_t);
extern void *allocarray_with(struct alloc *, size_t, size_t);
extern void *reallocarray_with(struct alloc *, void *, size_t, size_t, size_t);
extern void *try_allocarray_with(struct alloc *, size_t, size_t);
extern void *try_reallocarray_with(struct alloc *, void *, size_t, size_t, size_t);
extern void deallocarray_with(struct alloc *, void *, size_t, size_t);
extern void *allocate(size_t);
extern void *reallocate(void *, size_t, size_t);
extern void *try_allocate(size_t);
extern void *try_reallocate(void *, size_t, size_t);
extern void deallocate(void *, size_t);
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096ul
#endif
#ifndef VINE_NO_POISON
#ifdef __GNUC__
#pragma GCC poison malloc
#pragma GCC poison calloc
#pragma GCC poison realloc
#pragma GCC poison emalloc
#pragma GCC poison ecalloc
#pragma GCC poison erealloc
#pragma GCC poison free
#endif
#endif
