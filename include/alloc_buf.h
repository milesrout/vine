#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including alloc_buf.h"
#endif
#ifdef VINE_BUF_ALLOC_H_INCLUDED
#error "May not include alloc_buf.h more than once"
#endif
#define VINE_BUF_ALLOC_H_INCLUDED
struct buf_alloc {
	struct alloc ba_alloc;
	char *ba_last;
	char *ba_cur;
	size_t ba_cap;
};
extern void buf_alloc_init(struct buf_alloc *, char *, size_t);
