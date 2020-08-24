#ifdef VINE__MEMORY_H_INCLUDED
#error "May not include _memory.h more than once"
#endif
#define VINE__MEMORY_H_INCLUDED
extern void *allocate(size_t);
extern void *reallocate(void *, size_t, size_t);
extern void *try_allocate(size_t);
extern void *try_reallocate(void *, size_t, size_t);
extern void deallocate(void *, size_t);
#define PAGE_SIZE 4096ul
