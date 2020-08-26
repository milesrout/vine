#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including alloc_slab.h"
#endif
#ifndef VINE_ALLOC_BUF_H_INCLUDED
#error "Must include alloc_buf.h before including alloc_slab.h"
#endif
#ifdef VINE_ALLOC_SLAB_H_INCLUDED
#error "May not include alloc_slab.h more than once"
#endif
#define VINE_ALLOC_SLAB_H_INCLUDED
struct slab {
	struct slab *slab_next;
	struct buf_alloc slab_ba;
	char slab_buf[1];
};
#define SLAB_HEADER_SIZE (sizeof(struct slab) - 1)
struct slab_alloc {
	size_t sa_align, sa_size;
	void (*sa_init)(void *ptr);
	void (*sa_finish)(void *ptr);
	struct slab *sa_slabs;
};
extern void slab_alloc_init(struct slab_alloc *, size_t, size_t, void (*)(void *), void (*)(void *));
extern void slab_alloc_finish(struct slab_alloc *);
extern void *slab_object_create(struct slab_alloc *);
extern void slab_object_destroy(struct slab_alloc *, void *);
extern void test_slab(void);
