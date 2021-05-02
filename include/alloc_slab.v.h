//require alloc.h
//require alloc_buf.h
//provide alloc_slab.h
struct slab {
	struct slab     *slab_next;
	struct buf_alloc slab_alloc;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
	char             slab_buf[0];
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
};
#define SLAB_HEADER_SIZE (sizeof(struct slab) - 1)
struct slab_alloc {
	size_t sa_align, sa_size;
	void (*sa_init)(void *);
	void (*sa_finish)(void *);
	struct slab *sa_slabs;
};
extern void slab_alloc_init(struct slab_alloc *, size_t, size_t, void (*)(void *), void (*)(void *));
extern void slab_alloc_finish(struct slab_alloc *);
extern void *slab_object_create(struct slab_alloc *);
extern void slab_object_destroy(struct slab_alloc *, void *);
extern void test_slab(void);
