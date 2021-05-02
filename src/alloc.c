#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "abort.h"
#include "checked.h"
#define VINE_NO_POISON
#include "alloc.h"

void *
allocate_with(struct alloc *alloc, size_t m)
{
	void *p = alloc->alloc_vtable->avt_allocate(alloc, m);
	if (p == NULL) {
		abort_with_error("Memory allocation failure\n");
	}

	return p;
}

void *
try_allocate_with(struct alloc *alloc, size_t m)
{
	return alloc->alloc_vtable->avt_allocate(alloc, m);
}

void *
reallocate_with(struct alloc *alloc, void *q, size_t m, size_t n)
{
	void *p = alloc->alloc_vtable->avt_reallocate(alloc, q, m, n);
	if (p == NULL) {
		abort_with_error("Memory allocation failure\n");
	}

	return p;
}

void *
try_reallocate_with(struct alloc *alloc, void *q, size_t m, size_t n)
{
	return alloc->alloc_vtable->avt_reallocate(alloc, q, m, n);
}

void
deallocate_with(struct alloc *alloc, void *p, size_t m)
{
	alloc->alloc_vtable->avt_deallocate(alloc, p, m);
}

void *
allocarray_with(struct alloc *alloc, size_t m, size_t n)
{
	return allocate_with(alloc, mul_sz(m, n));
}

void *
try_allocarray_with(struct alloc *alloc, size_t m, size_t n)
{
	size_t s;

	if (try_mul_sz(m, n, &s))
		return NULL;

	return try_allocate_with(alloc, s);
}

void *
reallocarray_with(struct alloc *alloc, void *q, size_t m, size_t old, size_t new)
{
	return reallocate_with(alloc, q, m * old, mul_sz(m, new));
}

void *
try_reallocarray_with(struct alloc *alloc, void *q, size_t m, size_t old, size_t new)
{
	size_t s;

	if (try_mul_sz(m, new, &s))
		return NULL;

	return try_reallocate_with(alloc, q, m * old, s);
}

void
deallocarray_with(struct alloc *alloc, void *p, size_t m, size_t n)
{
	deallocate_with(alloc, p, mul_sz(m, n));
}

void *
try_allocate(size_t m)
{
	return try_allocate_with(&sys_alloc, m);
}

void *
allocate(size_t m)
{
	return allocate_with(&sys_alloc, m);
}

void *
try_reallocate(void *q, size_t m, size_t n)
{
	return try_reallocate_with(&sys_alloc, q, m, n);
}

void *
reallocate(void *q, size_t m, size_t n)
{
	return reallocate_with(&sys_alloc, q, m, n);
}

void
deallocate(void *p, size_t m)
{
	deallocate_with(&sys_alloc, p, m);
}
