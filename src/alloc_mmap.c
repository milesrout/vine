#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <valgrind/valgrind.h>

#include "abort.h"
#include "alloc.h"
#include "printf.h"
#include "log.h"

static void *
mmap_allocate(struct alloc *a, size_t m)
{
	void *p;
	(void)a;
	p = mmap(NULL, m, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	VALGRIND_MALLOCLIKE_BLOCK(p, m, 0, 0);
	log_debug("alloc_mmap", "Allocating 0x%llx bytes at %p\n", m, p);
	return p;
}

static void *
mmap_reallocate(struct alloc *a, void *q, size_t m, size_t n)
{
	void *p = mremap(q, m, n, 0 /* |MREMAP_MAYMOVE */);
	VALGRIND_RESIZEINPLACE_BLOCK(p, m, n, 0);
	(void)a;
	return p;
}

static void
mmap_deallocate(struct alloc *a, void *p, size_t n)
{
	int r;
	log_debug("alloc_mmap", "Deallocating 0x%llx bytes at %p\n", n, p);
	(void)a;
	r = munmap(p, n);
	VALGRIND_FREELIKE_BLOCK(p, 0);
	if (r != 0) {
		abort_with_error("munmap failed with arguments %p and %llu", p, n);
	}
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