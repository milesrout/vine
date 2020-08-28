#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "printf.h"
#include "alloc.h"
#include "types.h"
#include "splay.h"

static struct splay *do_splay(struct splay *t, u64 key);

struct splay *
splay_get(struct splay *t, u64 key)
{
	return do_splay(t, key);
}

void
splay_delete(struct splay *t, struct alloc *alloc)
{
	if (t == NULL)
		return;
	splay_delete(t->left, alloc);
	splay_delete(t->right, alloc);
	deallocate_with(alloc, t, sizeof *t);
}

static
struct splay *
do_splay(struct splay *t, u64 key)
{
	struct splay n, *l, *r, *y;

	/* NULL represents the empty splay - nothing to do */
	if (t == NULL)
		return NULL;

	/* maintain left (l), middle (t) and right (r) trees */
	n.left = n.right = NULL;
	l = r = &n;

	for (;;) {
		/* if the path to the key from here goes left */
		if (key < t->key) {
			/* if the key to splay is missing */
			if (t->left == NULL)
				break;
			/* if the path to the key from here goes left,left */
			if (key < t->left->key) {
				/* perform a zig */
				y = t->left;
				t->left = y->right;
				y->right = t;
				t = y;
				/* if the key to splay is missing */
				if (t->left == NULL)
					break;
			}
			r->left = t;
			r = t;
			t = t->left;
		} else if (key > t->key) {
			if (t->right == NULL)
				break;
			if (key > t->right->key) {
				y = t->right;
				t->right = y->left;
				y->left = t;
				t = y;
				if (t->right == NULL)
					break;
			}
			l->right = t;
			l = t;
			t = t->right;
		} else {
			break;
		}
	}
	l->right = t->left;
	r->left = t->right;
	t->left = n.right;
	t->right = n.left;
	return t;
}

struct splay *
splay_insert(struct splay *t, struct alloc *alloc, u64 key, void *value)
{
	struct splay *new = allocate_with(alloc, sizeof *new);

	new->key = key;
	new->value = value;

	if (t == NULL) {
		new->left = new->right = NULL;
		return new;
	}

	t = do_splay(t, key);
	if (key < t->key) {
		new->left = t->left;
		new->right = t;
		t->left = NULL;
		return new;
	} else if (key > t->key) {
		new->right = t->right;
		new->left = t;
		t->right = NULL;
		return new;
	} else {
		deallocate_with(alloc, new, sizeof *new);
		return NULL;
	}
}

struct splay *
splay_remove(struct splay *t, struct alloc *alloc, u64 key)
{
	struct splay *x;

	if (t == NULL)
		return NULL;

	t = do_splay(t, key);
	if (key != t->key) {
		return t;
	}

	if (t->left == NULL) {
		x = t->right;
	} else {
		x = do_splay(t->left, key);
		x->right = t->right;
	}

	deallocate_with(alloc, t, sizeof *t);
	return x;
}

void
print_splay_tree_preorder(struct splay *t)
{
	if (t == NULL)
		return;
	eprintf("[");
	eprintf("%lu ", t->key);
	print_splay_tree_preorder(t->left);
	eprintf(" ");
	print_splay_tree_preorder(t->right);
	eprintf("]");
}

void
print_splay_tree_inorder(struct splay *t)
{
	if (t == NULL)
		return;
	eprintf("[");
	print_splay_tree_inorder(t->left);
	eprintf(" %lu ", t->key);
	print_splay_tree_inorder(t->right);
	eprintf("]");
}

