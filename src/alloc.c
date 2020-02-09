#include <stdlib.h>
#include <string.h>
#include "_memory.h"

#include "alloc.h"

void *allocate_with(struct alloc *alloc, size_t m)
{
	alloc->vtable->allocate(alloc, m);
}

void *reallocate_with(struct alloc *alloc, void *q, size_t m, size_t n)
{

	alloc->vtable->reallocate(alloc, q, m, n);
}

void deallocate_with(struct alloc *alloc, void *p, size_t m)
{

	alloc->vtable->deallocate(alloc, p, m);
}

