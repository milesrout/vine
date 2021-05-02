#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "eprintf.h"
#include "abort.h"
#include "alloc.h"
#include "log.h"

static void *
mmap_allocate(struct alloc *a, size_t m)
{
	void *p;
	p = mmap(NULL, m, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS,
		-1, 0);
#ifdef USE_VALGRIND
	VALGRIND_MALLOCLIKE_BLOCK(p, m, 0, 0);
#endif
	log_debug("alloc_mmap", "Allocating %lu bytes at %p\n", m, p);
	(void)a;
	return p;
}

static void *
mmap_reallocate(struct alloc *a, void *q, size_t m, size_t n)
{
	void *p = mremap(q, m, n, 0 /* |MREMAP_MAYMOVE */);
#ifdef USE_VALGRIND
	VALGRIND_RESIZEINPLACE_BLOCK(p, m, n, 0);
#endif
	(void)a;
	log_debug("alloc_mmap", "Reallocating %lu -> %lu bytes at %p to %p\n",
		m, n, q, p);
	return p;
}

static void
mmap_deallocate(struct alloc *a, void *p, size_t n)
{
	int r;
	log_debug("alloc_mmap", "Deallocating %lu bytes at %p\n", n, p);
	r = munmap(p, n);
#ifdef USE_VALGRIND
	VALGRIND_FREELIKE_BLOCK(p, 0);
#endif
	if (r != 0) {
		abort_with_error(
			"munmap failed with arguments %p and %lu",
			p,
			n);
	}
	(void)a;
}

static
struct alloc_vtable
mmap_alloc_vtable = {
	&mmap_allocate,
	&mmap_reallocate,
	&mmap_deallocate
};

struct alloc
mmap_alloc = {&mmap_alloc_vtable};
