#ifdef VINE_ALLOC_H_INCLUDED
#error "May not include alloc.h more than once"
#endif
#define VINE_ALLOC_H_INCLUDED
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
#ifndef PAGE_SIZE
#define PAGE_SIZE ((void)"PAGE_SIZE is defined in memory.h")
#endif
