#ifndef VINE__MEMORY_H_INCLUDED
#error "Must include memory.h before including heapstring.h"
#endif
#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including heapstring.h"
#endif
#ifdef VINE_HEAPSTRING_H_INCLUDED
#error "May not include heapstring.h more than once"
#endif
#define VINE_HEAPSTRING_H_INCLUDED
struct heapstring {
	size_t hs_cap;
	char   hs_str[1];
};
extern struct heapstring *heapstring_create(size_t, struct alloc *);
extern struct heapstring *heapstring_expand(struct heapstring *, struct alloc *, size_t);
extern void heapstring_destroy(struct heapstring *, struct alloc *);
#define HEAPSTRING_PAGE_CAP (PAGE_SIZE - sizeof(struct heapstring) + 1)
