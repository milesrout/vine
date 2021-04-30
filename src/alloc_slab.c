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
#include "alloc_slab.h"
#include "memory.h"
#include "log.h"
#include "str.h"
#include "checked.h"

static struct slab *create_slab(struct slab *next, size_t align);
static void destroy_slab(struct slab *);
static void destroy_slabs(struct slab **);

void
slab_alloc_init(struct slab_alloc *sa, size_t align, size_t size,
	void (*init)(void *), void (*finish)(void *))
{
	sa->sa_align = align;
	sa->sa_size = size;
	sa->sa_init = init;
	sa->sa_finish = finish;
	sa->sa_slabs = NULL;
}

void
slab_alloc_finish(struct slab_alloc *sa)
{
	destroy_slabs(&sa->sa_slabs);
}

void *
slab_object_create(struct slab_alloc *sa)
{
	void *ptr;
	size_t size = sa->sa_size;
	size_t align = sa->sa_align;

	if (sa->sa_slabs == NULL) {
		sa->sa_slabs = create_slab(sa->sa_slabs, align);
		ptr = allocate_with(&sa->sa_slabs->slab_alloc.ba_alloc, size);
	} else {
		ptr = try_allocate_with(&sa->sa_slabs->slab_alloc.ba_alloc, size);
		if (ptr == NULL) {
			sa->sa_slabs = create_slab(sa->sa_slabs, align);
			ptr = allocate_with(&sa->sa_slabs->slab_alloc.ba_alloc, size);
		} 
	}
	sa->sa_init(ptr);
	return ptr;
}

void
slab_object_destroy(struct slab_alloc *sa, void *ptr)
{
	sa->sa_finish(ptr);
}

static
struct slab *
create_slab(struct slab *next, size_t align)
{
	char *ptr = allocate_with(&mmap_alloc, PAGE_SIZE);
	char *buf = align_ptr(ptr + SLAB_HEADER_SIZE, align);
	struct slab *slab = (struct slab *)ptr;
	slab->slab_next = next;
	buf_alloc_init(&slab->slab_alloc, buf, (size_t)(ptr + PAGE_SIZE - buf));
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

#define FOO_ALIGN __alignof__(struct foo)
#define FOO_SIZE sizeof(struct foo)

/* __attribute__((__packed__)) */
struct foo {
	struct string str;
	size_t y;
	char x;
};

static 
void
foo_init(void *foo)
{
	eprintf("foo_init %p\n", foo);
	string_init(&((struct foo *)foo)->str);
}

static
void
foo_finish(void *foo)
{
	eprintf("foo_finish %p\n", foo);
	string_finish(&((struct foo *)foo)->str);
}

void
test_slab(void)
{
	struct slab_alloc sa;
	struct foo *f[3];
	
	slab_alloc_init(&sa, FOO_ALIGN, FOO_SIZE, &foo_init, &foo_finish);

	eprintf("FOO_ALIGN=%lu\n", FOO_ALIGN);
	eprintf("FOO_SIZE=%lu\n", FOO_SIZE);

	f[0] = slab_object_create(&sa);
	f[1] = slab_object_create(&sa);
	f[2] = slab_object_create(&sa);
	slab_object_destroy(&sa, f[0]);
	slab_object_destroy(&sa, f[1]);
	slab_object_destroy(&sa, f[2]);

	slab_alloc_finish(&sa);
}
