#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#ifdef VINE_USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "abort.h"
#include "printf.h"
#include "alloc.h"
#include "alloc_buf.h"
#include "slab_pool.h"
#include "memory.h"
#include "log.h"
#include "str.h"
#include "checked.h"

static struct slab *create_slab(struct slab *next, size_t align);
static void destroy_slab(struct slab *);
static void destroy_slabs(struct slab **);

void
slab_pool_init(struct slab_pool *pool, size_t align, size_t size,
	void (*init)(void *), void (*finish)(void *))
{
	pool->sp_align = align;
	pool->sp_size = size;
	pool->sp_init = init;
	pool->sp_finish = finish;
	pool->sp_slabs = NULL;
}

void
slab_pool_finish(struct slab_pool *pool)
{
	destroy_slabs(&pool->sp_slabs);
}

void *
slab_object_create(struct slab_pool *pool)
{
	void *ptr;
	size_t size = pool->sp_size;
	size_t align = pool->sp_align;

	if (pool->sp_slabs == NULL) {
		pool->sp_slabs = create_slab(pool->sp_slabs, align);
		ptr = allocate_with(&pool->sp_slabs->slab_ba.ba_alloc, size);
	} else {
		ptr = try_allocate_with(&pool->sp_slabs->slab_ba.ba_alloc, size);
		if (ptr == NULL) {
			pool->sp_slabs = create_slab(pool->sp_slabs, align);
			ptr = allocate_with(&pool->sp_slabs->slab_ba.ba_alloc, size);
		} 
	}
	pool->sp_init(ptr);
	return ptr;
}

void
slab_object_destroy(struct slab_pool *pool, void *ptr)
{
	pool->sp_finish(ptr);
}

static
struct slab *
create_slab(struct slab *next, size_t align)
{
	char *ptr = allocate_with(&mmap_alloc, PAGE_SIZE);
	char *buf = align_ptr(ptr + SLAB_HEADER_SIZE, align);
	struct slab *slab = (struct slab *)ptr;
	slab->slab_next = next;
	buf_alloc_init(&slab->slab_ba, buf, (size_t)(ptr + PAGE_SIZE - buf));
	return slab;
}

static
void
destroy_slabs(struct slab **slabs)
{
	while (*slabs) {
		struct slab *next = (*slabs)->slab_next;
		destroy_slab(*slabs);
		*slabs = next;
	}
}

static
void
destroy_slab(struct slab *slab)
{
	deallocate_with(&mmap_alloc, slab, PAGE_SIZE);
}
