#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "_memory.h"
#include <stdio.h>
#include "printf.h"
#include "log.h"

#include "alloc.h"
#include "alloc_buf.h"

static void *
buf_allocate(struct alloc *alloc, size_t m)
{
	struct buf_alloc *ba = (struct buf_alloc *)alloc;
	if (ba->ba_cap < m) {
		return NULL;
	}

	ba->ba_last = ba->ba_cur;
	ba->ba_cap -= m;
	ba->ba_cur += m;

	log_info("buf_alloc", "Allocating %llu bytes at %p\n", m, ba->ba_last);

	return ba->ba_last;
}

static void *
buf_reallocate(struct alloc *alloc, void *q, size_t m, size_t n)
{
	struct buf_alloc *ba = (struct buf_alloc *)alloc;
	if (q == ba->ba_last) {
		ba->ba_cur = (char *)q + (n - m);
		ba->ba_cap -= (n - m);
		return q;
	} else {
		char *new = buf_allocate(alloc, n);
		if (new == NULL) {
			return NULL;
		}
		memcpy(new, q, m);
		return new;
	}
}

static void
buf_deallocate(struct alloc *alloc, void *p, size_t n)
{
	struct buf_alloc *ba = (struct buf_alloc *)alloc;
	if (p == ba->ba_last) {
		ba->ba_cur = p;
		ba->ba_cap += n;
	}
}

static struct alloc_vtable
buf_alloc_vtable = {
	buf_allocate,
	buf_reallocate,
	buf_deallocate
};

static struct alloc
ba_alloc = {&buf_alloc_vtable};

void
buf_alloc_init(struct buf_alloc *ba, char *buf, size_t cap)
{
	ba->ba_alloc = ba_alloc;
	ba->ba_last = buf;
	ba->ba_cur = buf;
	ba->ba_cap = cap;
}

