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
	log_debug("alloc_sys", "Allocating 0x%llx bytes at %p\n", m, p);
	return p;
}

static void *
sys_reallocate(struct alloc *a, void *q, size_t m, size_t n)
{
	void *p = realloc(q, n);
	(void)a;
	if (p != NULL && n > m) {
		memset((char *)p + m, 0, n - m);
	}
	return p;
}

static void
sys_deallocate(struct alloc *a, void *p, size_t n)
{
	/* At the moment we don't support the kind of complex size-aware allocators
	 * that require us to pass the size of the allocated chunk of memory to the
	 * free function.
	 */
	log_debug("alloc_sys", "Deallocating 0x%llx bytes at %p\n", n, p);
	(void)a;
	(void)n;
	free(p);
}

static struct alloc_vtable
sys_alloc_vtable = {
	&sys_allocate,
	&sys_reallocate,
	&sys_deallocate
};

struct alloc
sys_alloc = {&sys_alloc_vtable};
