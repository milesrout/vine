#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including slab_pool.h"
#endif
#ifndef VINE_ALLOC_BUF_H_INCLUDED
#error "Must include alloc_buf.h before including slab_pool.h"
#endif
#ifdef VINE_SLAB_POOL_H_INCLUDED
#error "May not include slab_pool.h more than once"
#endif
#define VINE_SLAB_POOL_H_INCLUDED
struct slab {
	struct slab *slab_next;
	struct buf_alloc slab_ba;
	char slab_buf[1];
};
#define SLAB_HEADER_SIZE (sizeof(struct slab) - 1)
struct slab_pool {
	size_t sp_align, sp_size;
	void (*sp_init)(void *ptr);
	void (*sp_finish)(void *ptr);
	struct slab *sp_slabs;
};
extern void slab_pool_init(struct slab_pool *, size_t, size_t, void (*)(void *), void (*)(void *));
extern void slab_pool_finish(struct slab_pool *);
extern void *slab_object_create(struct slab_pool *);
extern void slab_object_destroy(struct slab_pool *, void *);
