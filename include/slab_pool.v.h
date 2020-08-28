//require alloc.h
//require alloc_buf.h
//provide slab_pool.h
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
