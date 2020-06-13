#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"

#include <stdarg.h>
#include <stdio.h>
#include "printf.h"

#include "log.h"

static void *
sys_allocate(struct alloc *a, size_t m)
{
	void *p;

	(void)a;

	p = calloc(m, 1);
	log_debug("alloc_sys", "Allocating 0x%lx bytes at %p\n", m, p);
	return p;
}

static void *
sys_reallocate(struct alloc *a, void *q, size_t m, size_t n)
{
	void *p;

	(void)a;

	p = realloc(q, n);
	if (p != NULL && n > m) {
		memset((char *)p + m, 0, n - m);
	}
	return p;
}

static void
sys_deallocate(struct alloc *a, void *p, size_t n)
{
	log_debug("alloc_sys", "Deallocating 0x%lx bytes at %p\n", n, p);

	(void)a;

	/* The system allocator can't do size-aware deallocation and has to
	 * store the size of the allocation beside the allocation! As a result,
	 * we ignore the size argument to deallocate. Still useful for logging,
	 * though.
	 */
	free(p);
}

static
struct alloc_vtable
sys_alloc_vtable = {
	&sys_allocate,
	&sys_reallocate,
	&sys_deallocate
};

struct alloc
sys_alloc = {&sys_alloc_vtable};
