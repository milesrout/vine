#ifdef VINE_ALLOC_H_INCLUDED
#error "May not include alloc.h more than once"
#endif
#define VINE_ALLOC_H_INCLUDED
struct alloc;
struct alloc_vtable {
	void *(*allocate)(struct alloc *, size_t);
	void *(*reallocate)(struct alloc *, void *, size_t, size_t);
	void (*deallocate)(struct alloc *, void *, size_t);
};
struct alloc {
	struct alloc_vtable *vtable;
};
extern struct alloc sys_alloc;
extern struct alloc mmap_alloc;
extern void *allocate_with(struct alloc *, size_t);
extern void *reallocate_with(struct alloc *, void *, size_t, size_t);
extern void *try_allocate_with(struct alloc *, size_t);
extern void *try_reallocate_with(struct alloc *, void *, size_t, size_t);
extern void deallocate_with(struct alloc *, void *, size_t);
