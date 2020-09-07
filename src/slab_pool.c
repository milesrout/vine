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

#define SLAB_SIZE (4 * PAGE_SIZE)

static struct slab *create_slab(struct slab *next, size_t align);
static void destroy_slab(struct slab *);
static void destroy_slabs(struct slab **);

/*
 * TODO: use something a bit smarter than a buf_alloc (which only supports
 * freeing the most recently allocated allocation) so that we can free objects
 *
 * this is a low priority because slab_pool is currently only used with things
 * we never want to free individually, like text_piece and text_node, which
 * should only be freed when the entire text buffer they're associated with is
 * wiped away with :bw.
 */

void
slab_pool_init(struct slab_pool *pool, size_t size, size_t align,
	void (*init)(void *, void *), void (*finish)(void *))
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
slab_object_create(struct slab_pool *pool, void *data)
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
	if (pool->sp_init != NULL)
		pool->sp_init(ptr, data);
	return ptr;
}

void
slab_object_destroy(struct slab_pool *pool, void *ptr)
{
	/* TODO: actually free the object */
	if (pool->sp_finish != NULL)
		pool->sp_finish(ptr);
}

static
struct slab *
create_slab(struct slab *next, size_t align)
{
	char *ptr = allocate_with(&mmap_alloc, SLAB_SIZE);
	char *buf = align_ptr(ptr + SLAB_HEADER_SIZE, align);
	struct slab *slab = (struct slab *)ptr;
	slab->slab_next = next;
	buf_alloc_init(&slab->slab_ba, buf, (size_t)(ptr + SLAB_SIZE - buf));
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
	deallocate_with(&mmap_alloc, slab, SLAB_SIZE);
}
