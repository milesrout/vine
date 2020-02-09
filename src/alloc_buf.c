#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "_memory.h"

#include "alloc.h"
#include "alloc_buf.h"

static void *buf_allocate(struct alloc *alloc, size_t m)
{
	char *result;
	struct buf_alloc *ba = (struct buf_alloc *)alloc;
	if (ba->ba_cap < m) {
		return NULL;
	}

	result = ba->ba_buf;
	ba->ba_cap += m;
	ba->ba_buf += m;

	return result;
}

static void *buf_reallocate(struct alloc *alloc, void *q, size_t m, size_t n)
{
	(void)alloc;
	(void)q;
	(void)m;
	(void)n;
	return NULL;
}

static void buf_deallocate(struct alloc *alloc, void *p, size_t n)
{
	(void)alloc;
	(void)p;
	(void)n;
	return;
}

static struct alloc_vtable buf_alloc_vtable = {
	buf_allocate,
	buf_reallocate,
	buf_deallocate
};
static struct alloc ba_alloc = {&buf_alloc_vtable};

void buf_alloc_init(struct buf_alloc *ba, char *buf, size_t cap)
{
	ba->ba_alloc = ba_alloc;
	ba->ba_buf = buf;
	ba->ba_cap = cap;
}

