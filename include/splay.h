#ifndef VINE_PRINTF_H_INCLUDED
#error "Must include printf.h before including splay.h"
#endif
#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including splay.h"
#endif
#ifndef VINE_TYPES_H_INCLUDED
#error "Must include types.h before including splay.h"
#endif
#ifdef VINE_SPLAY_H_INCLUDED
#error "May not include splay.h more than once"
#endif
#define VINE_SPLAY_H_INCLUDED
struct splay {
	struct splay *left, *right;
	u64 key;
	void *value;
};
extern struct splay *splay_get(struct splay *, u64);
extern struct splay *splay_insert(struct splay *, struct alloc *, u64, void *);
extern struct splay *splay_remove(struct splay *, struct alloc *, u64);
extern void splay_delete(struct splay *, struct alloc *);

extern void print_splay_tree_inorder(struct splay *);
extern void print_splay_tree_preorder(struct splay *);
