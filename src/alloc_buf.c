#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "checked.h"
#include "memory.h"
#include "eprintf.h"
#include "log.h"
#include "alloc.h"
#include "alloc_buf.h"

static
void *
buf_allocate(struct alloc *alloc, size_t m)
{
	struct buf_alloc *ba = (struct buf_alloc *)alloc;
	m = align_sz(m, sizeof(void *));
	if (ba->ba_cap < m) {
		log_debug("alloc_buf", "Allocating %lu bytes failed (cap = %lu)\n",
			m, ba->ba_cap);
		return NULL;
	}

	ba->ba_last = ba->ba_cur;
	ba->ba_cap -= m;
	ba->ba_cur += m;

	log_debug("alloc_buf", "Allocating %lu bytes at %p\n", m, (void *)ba->ba_last);

	return ba->ba_last;
}

static
void *
buf_reallocate(struct alloc *alloc, void *q, size_t m, size_t n)
{
	struct buf_alloc *ba = (struct buf_alloc *)alloc;
	m = align_sz(m, sizeof(void *));
	n = align_sz(n, sizeof(void *));
	if (q == ba->ba_last) {
		ba->ba_cur = (char *)q + (n - m);
		if (ba->ba_cap < (n - m)) {
			log_debug("alloc_buf", "Reallocating %lu -> %lu bytes "
			                       "at %p failed (cap = %lu)\n",
				m, n, q, ba->ba_cap);
			return NULL;
		}
		ba->ba_cap -= (n - m);
		log_debug("alloc_buf", "Reallocating %lu -> %lu bytes at %p\n",
			m, n, q);
		return q;
	} else {
		char *new = buf_allocate(alloc, n);
		if (new == NULL) {
			return NULL;
		}
		memcpy(new, q, m);
		log_debug("alloc_buf", "Reallocating %lu -> %lu bytes at %p to %p\n",
			m, n, q, (void *)new);
		return new;
	}
}

static
void
buf_deallocate(struct alloc *alloc, void *p, size_t n)
{
	struct buf_alloc *ba = (struct buf_alloc *)alloc;
	n = align_sz(n, sizeof(void *));
	log_debug("alloc_buf", "Deallocating %lu bytes at %p\n", n, p);
	if (p == ba->ba_last) {
		ba->ba_cur = p;
		ba->ba_cap += n;
	}
}

static
struct alloc_vtable
buf_alloc_vtable = {
	&buf_allocate,
	&buf_reallocate,
	&buf_deallocate
};

static
struct alloc
ba_alloc = { &buf_alloc_vtable };

void
buf_alloc_init(struct buf_alloc *ba, char *buf, size_t cap)
{
	ba->ba_alloc = ba_alloc;
	ba->ba_last = buf;
	ba->ba_cur = buf;
	ba->ba_cap = cap;
}
