#include <stdlib.h>
#include <string.h>
#include "_memory.h"

#include "alloc.h"
#include "abort.h"

void *allocate_with(struct alloc *alloc, size_t m)
{
	void *p = alloc->vtable->allocate(alloc, m);
	if (p == NULL) {
		abort_with_error("Memory allocation failure\n");
	}

	return p;
}

void *try_allocate_with(struct alloc *alloc, size_t m)
{
	return alloc->vtable->allocate(alloc, m);
}

void *reallocate_with(struct alloc *alloc, void *q, size_t m, size_t n)
{
	void *p = alloc->vtable->reallocate(alloc, q, m, n);
	if (p == NULL) {
		abort_with_error("Memory allocation failure\n");
	}

	return p;
}

void *try_reallocate_with(struct alloc *alloc, void *q, size_t m, size_t n)
{
	return alloc->vtable->reallocate(alloc, q, m, n);
}

void deallocate_with(struct alloc *alloc, void *p, size_t m)
{
	alloc->vtable->deallocate(alloc, p, m);
}

