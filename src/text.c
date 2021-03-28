#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <termios.h>

#include "abort.h"
#include "alloc.h"
#include "alloc_buf.h"
#include "slab_pool.h"
#include "checked.h"
#include "types.h"
#include "printf.h"
#include "memory.h"
#include "text.h"
#include "hash.h"
#include "heapstring.h"

static void text_tree_init(struct text_tree *);
static void text_tree_finish(struct text_tree *);
static void text_tree_insert_childless_before(struct text_tree *, struct text_node *);
static void text_tree_insert_childless_after(struct text_tree *, struct text_node *);
static void text_tree_replace(struct text_tree *, struct text_node *);
static void text_tree_split(struct text *, size_t);
static struct text_node *text_tree_get(struct text_tree *, size_t);
static struct text_node *get_mutable_piece(struct text *, size_t);
static void do_splay(struct text_tree *, size_t);
static void print_text_tree_inorder(struct text_node *, size_t, size_t);
static void insert_at(struct text *tx, size_t cursor, char *buf, size_t len);
static void insert_node_at(struct text *tx, size_t cursor, struct text_node *P);
static void text_tree_insert_leftchild_after(struct text_tree *tree, struct text_node *new);
static void text_tree_insert_leftchild_before(struct text_tree *tree, struct text_node *new);
static struct text_node *create_node(struct text *tx, struct text_piece *p);
static struct text_piece *create_piece(struct text *tx);
static size_t true_size(struct text_node *n);
static void copy_to_buffer(struct text_node *t, size_t first, size_t last, char *buf, size_t cap, size_t *copied);

#define NODE_SIZE sizeof(struct text_node)
#define NODE_ALIGN __alignof__(struct text_node)
#define PIECE_SIZE sizeof(struct text_piece)
#define PIECE_ALIGN __alignof__(struct text_piece)
#define BUFFER_SIZE PAGE_SIZE

static
void
text_node_init(struct text_node *n, struct text_piece *p)
{
	n->txn_parent = NULL;
	n->txn_left = NULL;
	n->txn_left_size = 0;
	n->txn_right = NULL;
	n->txn_right_size = 0;
	n->txn_piece = p;
}

#define BLUE "cornflowerblue"
#define RED  "crimson"
static
log_node(FILE *f, struct text_node *n)
{
	efprintf(f, "        \"%p\" [label=\"", n);

	if (n->txn_left == NULL) {
		efprintf(f, "%lu:", n->txn_left_size);
	}
	efprintf(f, "%lx:%.*s",
		(0xffff & (u64)n), n->txn_piece->txp_len, n->txn_piece->txp_buf);
	if (n->txn_right == NULL) {
		efprintf(f, ":%lu",
			n->txn_right_size);
	}
	efprintf(f, "\",color=\"%s\"];\n",
		n->txn_piece->txp_state ? RED : BLUE);

	if (n->txn_left) {
		efprintf(f, "        \"%p\" -> \"%p\" [label=\"%lu\"];\n",
			n, n->txn_left, n->txn_left_size);
		log_node(f, n->txn_left);
	}
	if (n->txn_right) {
		efprintf(f, "        \"%p\" -> \"%p\" [label=\"%lu\"];\n",
			n, n->txn_right, n->txn_right_size);
		log_node(f, n->txn_right);
	}
	if (n->txn_parent) {
		efprintf(f, "        \"%p\" -> \"%p\" [style=\"dotted\"];\n",
			n, n->txn_parent);
	}
}

static
log_tree_n(FILE *f, struct text_node *n)
{
	efprintf(f, "digraph vine {\n");
	efprintf(f, "        graph [ordering=\"out\"];\n");
	efprintf(f, "        ratio = fill;\n");
	efprintf(f, "        node [style=\"filled\"];\n");
	log_node(f, n);
	efprintf(f, "}\n");
}

static
log_tree(FILE *f, struct text_tree *t)
{
	efprintf(f, "digraph vine {\n");
	efprintf(f, "        graph [ordering=\"out\"];\n");
	efprintf(f, "        ratio = fill;\n");
	efprintf(f, "        node [style=\"filled\"];\n");
	log_node(f, t->txt_root);
	efprintf(f, "}\n");
}

static
void
check_parents_are_correct(struct text_node *n)
{
	if (n->txn_left != NULL) {
		assert1(n == n->txn_left->txn_parent);
		check_parents_are_correct(n->txn_left);
	}
	if (n->txn_right != NULL) {
		assert1(n == n->txn_right->txn_parent);
		check_parents_are_correct(n->txn_right);
	}
}

static
size_t
true_size(struct text_node *n)
{
	size_t result = 0;

	if (n != NULL) {
		if (n->txn_left != NULL)
			result += true_size(n->txn_left);
		if (n->txn_right != NULL)
			result += true_size(n->txn_left);
		result += n->txn_piece->txp_len;
	}

	return result;
}

static
size_t
check_sizes_are_correct(struct text_node *n)
{
	if (n->txn_left == NULL) {
		assert1(n->txn_left_size == 0);
	} else {
		assert1(n->txn_left_size == check_sizes_are_correct(n->txn_left));
	}
	if (n->txn_right == NULL) {
		assert1(n->txn_right_size == 0);
	} else {
		assert1(n->txn_right_size == check_sizes_are_correct(n->txn_right));
	}
	return n->txn_left_size + n->txn_right_size + n->txn_piece->txp_len;
}

volatile int g_dobreak = 0;

static
void
log_the_tree(struct text_node *n)
{
	FILE *f = fopen("out.dot", "w");
	log_tree_n(f, n);
	fclose(f);
	if (g_dobreak)
		breakpoint();
	efprintf(stderr, "LOGGED TREE\n");
}

static
void
check_tree_invariants(struct text_node *n)
{
	log_the_tree(n);
	check_parents_are_correct(n);
	(void)check_sizes_are_correct(n);
}

static
void
check_invariants(struct text *tx)
{
	check_tree_invariants(tx->tx_tree.txt_root);
}

/*
 * INVARIANTS:
 *
 * IF   (tx_add_buf != NULL)
 * THEN (tx_tree.txt_root->txn_piece->txp_state == TXP_CONST)
 *
 * IF   (n->txn_piece->txp_state == TXP_MUTABLE)
 * THEN (n->txn_parent == NULL && tx_add_buf == NULL)
 */
void
text_init(struct text *tx, struct alloc *alloc, char *buf, size_t len)
{
	struct text_piece *orig;

	tx->tx_alloc = alloc;

	/* create the object pools */
	slab_pool_init(&tx->tx_nodes_pool, NODE_SIZE, NODE_ALIGN, (void(*)(void *, void *))text_node_init, NULL);
	slab_pool_init(&tx->tx_pieces_pool, PIECE_SIZE, PIECE_ALIGN, NULL, NULL);

	efprintf(stderr, "text_node: %lu @ %lu\n", NODE_SIZE, NODE_ALIGN);
	efprintf(stderr, "text_piece: %lu @ %lu\n", PIECE_SIZE, PIECE_ALIGN);

	/* create a text piece for the original buffer content */
	orig = create_piece(tx);
	orig->txp_buf = buf;
	orig->txp_len = len;
	orig->txp_state = TXP_CONST;

	tx->tx_orig_buf = buf;
	tx->tx_orig_buf_len = len;
	tx->tx_add_buf = NULL;
	tx->tx_add_buf_cap = 0;

	text_tree_init(&tx->tx_tree);
	tx->tx_tree.txt_root = create_node(tx, orig);
}

void
text_finish(struct text *tx)
{
	slab_pool_finish(&tx->tx_nodes_pool);
	slab_pool_finish(&tx->tx_pieces_pool);
}

static
struct text_node *
create_node(struct text *tx, struct text_piece *p)
{
	return (struct text_node *)slab_object_create(&tx->tx_nodes_pool, p);
}

static
struct text_piece *
create_piece(struct text *tx)
{
	return (struct text_piece *)slab_object_create(&tx->tx_pieces_pool, NULL);
}

static
char *
create_buffer(struct text *tx)
{
	return allocate_with(&mmap_alloc, BUFFER_SIZE);
}

static
void
finish_root_piece(struct text *tx)
{
	struct text_piece *p = tx->tx_tree.txt_root->txn_piece;

	/* nothing to do */
	if (p->txp_state == TXP_CONST)
		return;

	assert1(tx->tx_add_buf == NULL);
	tx->tx_add_buf = p->txp_buf + p->txp_len;
	tx->tx_add_buf_cap = p->txp_cap;
	p->txp_state = TXP_CONST;
}

#define TM_NORMAL 0
#define TM_INSERT 1
#define TM_VISUAL 2

void
test_text(void)
{
	int mode = TM_NORMAL;
	struct text tx;
	char buf[] = "hello, world!";
	size_t cursor = 0, vis_cursor = 0;
	size_t size = sizeof(buf) - 1;
	static struct termios oldterm, newterm;
	struct heapstring *reg = NULL;
	size_t reg_len = 0;

	eprintf("piece %lux%lu=%lu\n", sizeof(struct text_piece), PAGE_SIZE /
			sizeof(struct text_piece), PAGE_SIZE / sizeof(struct
				text_piece) * sizeof(struct text_piece));
	eprintf("node %lux%lu=%lu\n", sizeof(struct text_node), PAGE_SIZE /
			sizeof(struct text_node), PAGE_SIZE / sizeof(struct
				text_node) * sizeof(struct text_node));

	tcgetattr(STDIN_FILENO, &oldterm);
	newterm = oldterm;
	newterm.c_lflag &= (unsigned)~(ICANON);
	tcsetattr(STDIN_FILENO, TCSANOW, &newterm);

	text_init(&tx, &mmap_alloc, buf, sizeof(buf) - 1);
	eprintf("%.*s|%s\n", (int)cursor, buf, buf + cursor);
	for (;;) {
		int c;
		check_invariants(&tx);
		c = getchar();
		if (mode == TM_INSERT) {
			if (c == 0x7f) {
				finish_root_piece(&tx);
				mode = TM_NORMAL;
			} else if (c != 0x1b) {
				char ch = (char)c;
				insert_at(&tx, cursor, &ch, 1);
				cursor++;
				size++;
			} else {
				finish_root_piece(&tx);
				mode = TM_NORMAL;
			}
		} else if (mode == TM_VISUAL) {
			if (c == 'l') {
				cursor = (cursor + 1) % size;
			} else if (c == 'h') {
				cursor = (cursor - 1) % size;
			} else if (c == 'y') {
				size_t vis_first, vis_len, vis_last;
				size_t copied;
				if (cursor > vis_cursor) {
					vis_len = cursor - vis_cursor; 
					vis_last = cursor;
					vis_first = vis_cursor;
				} else {
					vis_len = vis_cursor - cursor;
					vis_first = cursor;
					vis_last = vis_cursor;
				}

				if (reg != NULL)
					heapstring_destroy(reg, &sys_alloc);
				reg = heapstring_create(vis_len, &sys_alloc);
				copy_to_buffer(tx.tx_tree.txt_root,
					vis_first, vis_last,
					reg->hs_str, reg->hs_cap,
					&copied);
				reg_len = copied;
				eprintf("register(%lu==%lu)=[%.*s\n].",
					vis_len, reg_len, vis_len, reg->hs_str);
				mode = TM_NORMAL;
			} else {
				mode = TM_NORMAL;
			}
		} else {
			if (c == EOF || c == 'q') {
				break;
			} else if (c == 'v') {
				vis_cursor = cursor;
				mode = TM_VISUAL;
			} else if (c == 'l') {
				cursor = (cursor + 1) % size;
			} else if (c == 'h') {
				cursor = (cursor - 1) % size;
			} else if (c == 'i') {
				mode = TM_INSERT;
			} else if (c == 'p') {
				insert_at(&tx, cursor, reg->hs_str, reg_len);
				size += reg_len;
				finish_root_piece(&tx);
			} else if (c == 'P') {
				size_t loc = cursor == 0 ? 0 : cursor - 1;
				insert_at(&tx, loc, reg->hs_str, reg_len);
				size += reg_len;
				cursor += reg_len;
				finish_root_piece(&tx);
			}
		}
		eprintf("%lu %lu", cursor, size);
		print_text_tree_inorder(tx.tx_tree.txt_root, cursor, 0);
		eprintf("\n");
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
	text_finish(&tx);
}

/* Return a struct text_node at the cursor that we are allowed to append to
static
struct text_node *
get_mutable_piece(struct text *tx, size_t cursor)
{
	struct text_node *n;

	 * splay the node corresponding to the current cursor position * 
	n = text_tree_get(&tx->tx_tree, cursor);

	 * if the cursor points to the end of the currently active piece * 
	if (n->txn_piece == tx->tx_active_piece)
		if (cursor == n->txn_left_size + n->txn_piece->txp_len)
			return n;

	 * otherwise, we need to create a new editable piece and make it the
	 * current piece * 
	text_tree_split(tx, cursor);

	tx->tx_active_piece = tx->tx_tree.txt_root->txn_piece;
	return tx->tx_tree.txt_root;
}
 */

/* This is a rather complicated function because it has to handle a combination
 * of cases.  When text is inserted at a position, there are a couple of
 * possibilities.
 *
 * The simplest is that the text is being inserted at the end of the current,
 * mutable text piece.  If there is room, the new text is just inserted at the
 * end.  If there isn't room for all of it, as much is inserted into that piece
 * as can be, then the piece is made immutable and a new piece is created for
 * the rest.
 *
 * When text is not being inserted at the end of the current and mutable text
 * piece, we need to create a new text piece for the text to be inserted,
 * wherever it is.  If the current piece is mutable, then it is made immutable
 * and any space remaining in it is saved as the 'add buffer'.
 *
 * When we create a new mutable text piece, we first check if there is an 'add  
 * buffer' saved, and use that as the underlying buffer.  If there isn't, then
 * we create a new buffer.  This is done in text_tree_split.
 *
 * In other words, we try the following in order:
 * - insert at the end of the current mutable piece, OR
 * - insert in a new piece created from tx_add_buf, OR
 * - insert in a new piece created from a new buffer
 */
static
void
insert_at(struct text *tx, size_t cursor, char *buf, size_t len)
{
	struct text_node *n = text_tree_get(&tx->tx_tree, cursor);
	struct text_piece *p = n->txn_piece;
	struct text_piece *q = NULL;

	/* if the cursor points to a mutable piece */
	if (p->txp_state == TXP_MUTABLE) {
		/* and the cursor points to the end of that piece */
		if (cursor == n->txn_left_size + n->txn_piece->txp_len) {
			/* and that piece has room for the new data */
			if (len <= p->txp_cap) {
				/* copy the data into the existing buffer */
				memcpy(p->txp_buf + p->txp_len, buf, len);
				p->txp_len += len;
				p->txp_cap -= len;
				if (p->txp_cap == 0)
					p->txp_state = TXP_CONST;
				return;
			} else {
				/* copy as much data as possible */
				memcpy(p->txp_buf + p->txp_len, buf, p->txp_cap);
				len -= p->txp_cap;
				/* update our cursor to point to the position
				 * that we will be inserting the remaining
				 * text.  we do not need to retain the old
				 * cursor because the text we are inserting
				 * here is going directly into the tree i.e. we
				 * don't need to insert it later with
				 * a cursor. */
				cursor += p->txp_cap;
				buf += p->txp_cap;
				p->txp_len += p->txp_cap;
				p->txp_state = TXP_CONST;
				p->txp_cap = 0;
			}
		} else {
			/* the cursor points to the middle of the current
			 * mutable piece, so we'll have to split it.  we'll
			 * make it constant and make its spare capacity the
			 * 'add buffer' now so that we don't have to do this
			 * later when it gets split */
			tx->tx_add_buf = p->txp_buf + p->txp_len;
			tx->tx_add_buf_cap = p->txp_cap;
			p->txp_state = TXP_CONST;
		}
	}

	/* if we get here, then the cursor points into an immutable piece,
	 * either because it was that way already or because we just made it
	 * immutable.
	 */

	/* if there is an add buffer */
	if (tx->tx_add_buf != NULL) {
		/* the piece for the 'add buffer' part of the inserted text */
		q = create_piece(tx);
		/* if the add buffer has sufficient capacity */
		if (tx->tx_add_buf_cap >= len) {
			/* copy the data into the existing buffer */
			memcpy(tx->tx_add_buf, buf, len);
			q->txp_buf = tx->tx_add_buf;
			q->txp_len = len;
			q->txp_cap = tx->tx_add_buf_cap - len;
			q->txp_state = q->txp_cap == 0
				? TXP_CONST
				: TXP_MUTABLE;
			tx->tx_add_buf = NULL;
			tx->tx_add_buf_cap = 0;
			insert_node_at(tx, cursor, create_node(tx, q));
			return;
		}
		/* the add buffer has insufficient capacity */
		else {
			/* copy as much data as possible */
			memcpy(tx->tx_add_buf, buf, tx->tx_add_buf_cap);
			len -= tx->tx_add_buf_cap;
			q->txp_buf = tx->tx_add_buf;
			q->txp_len = tx->tx_add_buf_cap;
			q->txp_cap = 0;
			q->txp_state = TXP_CONST;
			tx->tx_add_buf = NULL;
			tx->tx_add_buf_cap = 0;
		}
	}

	/* if we get here, then the cursor points into an immutable piece and
	 * the add buffer doesn't exist.  there may or may not be an immutable
	 * piece `*q' which contains the initial part of the text we need to
	 * insert.  if there isn't, `q' will be NULL.
	 */

	/* create a new buffer for the remaining text to be inserted */
	{
		/* we will overallocate to the nearest BUFFER_SIZE, but worry
		 * not, this buffer will be reused if needed
		 */
		size_t bufsz = align_sz(len, BUFFER_SIZE);
		char *newbuf = allocate_with(&mmap_alloc, bufsz);
		memcpy(newbuf, buf, len);
		p = create_piece(tx);
		p->txp_buf = newbuf;
		p->txp_len = len;
		p->txp_cap = bufsz - len;
		p->txp_state = p->txp_cap == 0 ? TXP_CONST : TXP_MUTABLE;
	}

	{
		struct text_node *P = create_node(tx, p);
		struct text_node *Q = q == NULL ? NULL : create_node(tx, q);
		size_t left_edge_pos = n->txn_left_size;
		size_t right_edge_pos = n->txn_left_size +
			n->txn_piece->txp_len;

		if (cursor == left_edge_pos) {
			if (q == NULL) {
				text_tree_insert_childless_before(&tx->tx_tree, P);
			} else {
				P->txn_left = Q;
				P->txn_left_size = Q->txn_piece->txp_len;
				Q->txn_parent = P;
				text_tree_insert_leftchild_before(&tx->tx_tree, P);
			}
		} else if (cursor == right_edge_pos) {
			if (q == NULL) {
				text_tree_insert_childless_after(&tx->tx_tree, P);
			} else {
				P->txn_left = Q;
				P->txn_left_size = Q->txn_piece->txp_len;
				Q->txn_parent = P;
				text_tree_insert_leftchild_after(&tx->tx_tree, P);
			}
		} else if (cursor < left_edge_pos || cursor > right_edge_pos) {
			breakpoint();
			/* abort_with_error("insert_at: This case should be impossible"); */
		} else {
			struct text_piece *l = create_piece(tx);
			struct text_piece *r = create_piece(tx);
			struct text_node *L = create_node(tx, l);
			struct text_node *R = create_node(tx, r);

			/* how far into the node the cursor is */
			size_t offset = cursor - left_edge_pos;

			l->txp_buf = n->txn_piece->txp_buf;
			l->txp_len = offset;
			l->txp_state = TXP_CONST;
			r->txp_buf = n->txn_piece->txp_buf + offset;
			r->txp_len = n->txn_piece->txp_len - offset;
			r->txp_state = TXP_CONST;

			L->txn_left = L->txn_right = NULL;
			R->txn_left = R->txn_right = NULL;
			L->txn_left_size = L->txn_right_size = 0;
			R->txn_left_size = R->txn_right_size = 0;
			L->txn_piece = l;
			R->txn_piece = r;

			P->txn_left = L;
			P->txn_left_size = l->txp_len;
			L->txn_parent = P;

			P->txn_right = R;
			P->txn_right_size = r->txp_len;
			R->txn_parent = P;

			if (q != NULL) {
				L->txn_right = Q;
				L->txn_right_size = Q->txn_piece->txp_len;
				Q->txn_parent = L;

				P->txn_left_size += Q->txn_piece->txp_len;
			}

			text_tree_replace(&tx->tx_tree, P);
		}
	}
}

static
void
insert_node_at(struct text *tx, size_t cursor, struct text_node *P)
{
	struct text_node *root = tx->tx_tree.txt_root;

	/* because n is the root, its 'left size' is the size of all the text
	 * on the left of this node i.e. the offset of the text in this node
	 * from the beginning of the document */
	size_t left_edge_pos = root->txn_left_size;

	/* same logic */
	size_t right_edge_pos = root->txn_left_size + root->txn_piece->txp_len;

	/*breakpoint();*/

	/* XXX: is it possible that either the left_edge or the right_edge case
	 * is unnecessary?  after all, in nearly every case that we are at the
	 * left edge of a node we are also at the right edge of the previous
	 * node and vice versa.  however, when we are at the beginning
	 * (resp. end) there is no node before (resp. after) the node that will
	 * get splayed to the top. 
	 * possibly this is where having sentinel nodes really helps, because
	 * it means that even when we are at the 'beginning' we're still
	 * between two nodes and only need to consider one of these two cases.
	 * c'est la vie. */
	if (cursor == left_edge_pos) {
		/* the cursor points directly before n.  we want to insert the
		 * new node directly before n. */
		text_tree_insert_childless_before(&tx->tx_tree, P);
	} else if (cursor == right_edge_pos) {
		/* the cursor points directly after n.  we want to insert the
		 * new node directly after n. */
		text_tree_insert_childless_after(&tx->tx_tree, P);
	} else if (cursor < left_edge_pos || cursor > right_edge_pos) {
		breakpoint();
		abort_with_error("This case should not be possible");
	} else { /* cursor > left_edge_pos && cursor < right_edge_pos */
		/* the trickier case.  the cursor points within n.  we want to
		 * create two new nodes corresponding to everything in
		 * n before the cursor and everything in n after the
		 * cursor. then we want to insert the three new nodes, making
		 * sure to insert the new blank and editable node last (or at
		 * least that it is the root at the end - insert all three at
		 * once and then splay it up perhaps? */
		struct text_piece *l = create_piece(tx);
		struct text_piece *r = create_piece(tx);
		struct text_node *L = create_node(tx, l);
		struct text_node *R = create_node(tx, r);

		/* how far into the node the cursor is */
		size_t offset = cursor - left_edge_pos;

		l->txp_buf = root->txn_piece->txp_buf;
		l->txp_len = offset;
		l->txp_state = TXP_CONST;
		r->txp_buf = root->txn_piece->txp_buf + offset;
		r->txp_len = root->txn_piece->txp_len - offset;
		r->txp_state = TXP_CONST;

		L->txn_left = L->txn_right = NULL;
		R->txn_left = R->txn_right = NULL;
		L->txn_left_size = L->txn_right_size = 0;
		R->txn_left_size = R->txn_right_size = 0;
		L->txn_piece = l;
		R->txn_piece = r;

		P->txn_left = L;
		P->txn_right = R;
		P->txn_left_size = l->txp_len;
		P->txn_right_size = r->txp_len;
		L->txn_parent = P;
		R->txn_parent = P;

		text_tree_replace(&tx->tx_tree, P);
	}
}

/* Split the root node at the position `cursor' in order to insert the new text
 * piece `piece'.  If the cursor points immediately before or immediately after
 * the root node, it isn't split and the new node is simply inserted directly
 * before or after the root node.  Requires that cursor points into,
 * immediately after or immediately before the node to split.  Requires that
 * the node to split is at the root of the tree.
static
void
text_tree_split(struct text *tx, size_t cursor, struct text_piece *piece)
{
	struct text_node *root = tx->tx_tree.txt_root;
	struct text_node *P;

	* because n is the root, its 'left size' is the size of all the text
	 * on the left of this node i.e. the offset of the text in this node
	 * from the beginning of the document *
	size_t left_edge_pos = root->txn_left_size;

	* same logic *
	size_t right_edge_pos = root->txn_left_size + root->txn_piece->txp_len;

	* this will be the new root node at the end of this operation *
	P = create_node(tx, piece);

	* XXX: is it possible that either the left_edge or the right_edge case
	 * is unnecessary?  after all, in nearly every case that we are at the
	 * left edge of a node we are also at the right edge of the previous
	 * node and vice versa.  however, when we are at the beginning
	 * (resp. end) there is no node before (resp. after) the node that will
	 * get splayed to the top. 
	 * possibly this is where having sentinel nodes really helps, because
	 * it means that even when we are at the 'beginning' we're still
	 * between two nodes and only need to consider one of these two cases.
	 * c'est la vie. *
	if (cursor == left_edge_pos) {
		* the cursor points directly before n.  we want to insert the
		 * new node directly before n. *
		text_tree_insert_childless_before(&tx->tx_tree, P);
	} else if (cursor == right_edge_pos) {
		* the cursor points directly after n.  we want to insert the
		 * new node directly after n. *
		text_tree_insert_childless_after(&tx->tx_tree, P);
	} else if (cursor < left_edge_pos || cursor > right_edge_pos) {
		abort_with_error("This case should not be possible");
	} else { * cursor > left_edge_pos && cursor < right_edge_pos *
		* the trickier case.  the cursor points within n.  we want to
		 * create two new nodes corresponding to everything in
		 * n before the cursor and everything in n after the
		 * cursor. then we want to insert the three new nodes, making
		 * sure to insert the new blank and editable node last (or at
		 * least that it is the root at the end - insert all three at
		 * once and then splay it up perhaps? *
		struct text_piece *l = create_piece(tx);
		struct text_piece *r = create_piece(tx);
		struct text_node *L = create_node(tx, l);
		struct text_node *R = create_node(tx, r);

		* how far into the node the cursor is *
		size_t offset = cursor - left_edge_pos;

		l->txp_buf = root->txn_piece->txp_buf;
		l->txp_len = offset;
		l->txp_state = TXP_CONST;
		r->txp_buf = root->txn_piece->txp_buf + offset;
		r->txp_len = root->txn_piece->txp_len - offset;
		r->txp_state = TXP_CONST;

		L->txn_left = L->txn_right = NULL;
		R->txn_left = R->txn_right = NULL;
		L->txn_left_size = L->txn_right_size = 0;
		R->txn_left_size = R->txn_right_size = 0;
		L->txn_piece = l;
		R->txn_piece = r;

		P->txn_left = L;
		P->txn_right = R;
		P->txn_left_size = l->txp_len;
		P->txn_right_size = r->txp_len;
		L->txn_parent = P;
		R->txn_parent = P;

		text_tree_replace(&tx->tx_tree, P);
	}
}
 */

static
void
text_tree_insert_leftchild_before(struct text_tree *tree, struct text_node *new)
{
	struct text_node *root = tree->txt_root;

	new->txn_left->txn_left = root->txn_left;
	new->txn_left->txn_left_size = root->txn_left_size;
	new->txn_left_size += root->txn_left_size;
	new->txn_right = root;
	new->txn_right_size = root->txn_right_size + root->txn_piece->txp_len;

	root->txn_left = NULL;
	root->txn_left_size = 0;

	if (new->txn_left->txn_left)
		new->txn_left->txn_left->txn_parent = new->txn_left;
	if (new->txn_right)
		new->txn_right->txn_parent = new;

	tree->txt_root = new;
}


static
void
text_tree_insert_leftchild_after(struct text_tree *tree, struct text_node *new)
{
	struct text_node *root = tree->txt_root;

	new->txn_right->txn_right = root->txn_right;
	new->txn_right->txn_right_size = root->txn_right_size;
	new->txn_right_size += root->txn_right_size;
	new->txn_left = root;
	new->txn_left_size = root->txn_left_size + root->txn_piece->txp_len;

	root->txn_right = NULL;
	root->txn_right_size = 0;

	if (new->txn_left)
		new->txn_left->txn_parent = new;
	if (new->txn_right->txn_right)
		new->txn_right->txn_right->txn_parent = new->txn_right;

	tree->txt_root = new;
}

/* Add a new childless node immediately before the root node in the tree */
static
void
text_tree_insert_childless_before(struct text_tree *tree, struct text_node *new)
{
	struct text_node *root = tree->txt_root;

	new->txn_left = root->txn_left;
	new->txn_left_size = root->txn_left_size;
	new->txn_right = root;
	new->txn_right_size = root->txn_right_size + root->txn_piece->txp_len;

	root->txn_left = NULL;
	root->txn_left_size = 0;

	if (new->txn_left)
		new->txn_left->txn_parent = new;
	if (new->txn_right)
		new->txn_right->txn_parent = new;

	tree->txt_root = new;
}


/* Add a new childless node immediately after the root node in the tree */
static
void
text_tree_insert_childless_after(struct text_tree *tree, struct text_node *new)
{
	struct text_node *root = tree->txt_root;

	new->txn_right = root->txn_right;
	new->txn_right_size = root->txn_right_size;
	new->txn_left = root;
	new->txn_left_size = root->txn_left_size + root->txn_piece->txp_len;

	root->txn_right = NULL;
	root->txn_right_size = 0;

	if (new->txn_left)
		new->txn_left->txn_parent = new;
	if (new->txn_right)
		new->txn_right->txn_parent = new;

	tree->txt_root = new;
}

/* Replace the root node of the tree with 'new', which must have exactly two
 * children, the left of which must not have a left child and the right of
 * which must not have a right child.
 *
 *              N               N
 *    A        / \             / \
 *   / \   +  L   R    -->    L   R
 *  B   C      \ /           / \ / \
 *             ? ?          B  ? ?  C
 *
 *            thus
 *
 *                              N
 *    A         N              / \
 *   / \   +   / \     -->    L   R
 *  B   C     L   R          /     \
 *                          B       C
 *
 *             and
 *
 *              N               N
 *    A        / \             / \
 *   / \   +  L   R    -->    L   R
 *  B   C      \             / \   \
 *              Q           B   Q   C
 */
static
void
text_tree_replace(struct text_tree *tree, struct text_node *new)
{
	new->txn_left->txn_left = tree->txt_root->txn_left;
	new->txn_left->txn_left_size = tree->txt_root->txn_left_size;
	new->txn_left_size += tree->txt_root->txn_left_size;

	new->txn_right->txn_right = tree->txt_root->txn_right;
	new->txn_right->txn_right_size = tree->txt_root->txn_right_size;
	new->txn_right_size += tree->txt_root->txn_right_size;

	if (new->txn_left->txn_left)
		new->txn_left->txn_left->txn_parent = new->txn_left;
	if (new->txn_right->txn_right)
		new->txn_right->txn_right->txn_parent = new->txn_right;

	tree->txt_root = new;
	check_tree_invariants(tree->txt_root);
}

static
struct text_node *
text_tree_get(struct text_tree *tree, size_t cursor)
{
	do_splay(tree, cursor);
	return tree->txt_root;
}

/*
static
size_t
parental_left_offset(struct text_node *n)
{
	if (n->txn_parent == NULL)
		return 0;

	if (n == n->txn_parent->txn_right) {
		return parental_left_offset(n->txn_parent) +
			n->txn_parent->txn_left_size +
			n->txn_parent->txn_piece->txp_len;
	} else if (n == n->txn_parent->txn_left) {
		return parental_left_offset(n->txn_parent);
	} else {
		*abort_with_error("This case should not be possible");*
		return 0;
	}
}
*/

static
size_t
left_edge(struct text_node *n)
{
	return n->txn_left_size;
}

static
size_t
right_edge(struct text_node *n)
{
	return n->txn_left_size + n->txn_piece->txp_len;
}

static
size_t
calc_size(struct text_node *n)
{
	if (n == NULL)
		return 0;
	return add_sz(
		add_sz(n->txn_piece->txp_len, n->txn_left_size),
		n->txn_right_size);
}

static
void
do_splay(struct text_tree *tree, size_t cursor)
{
	struct text_node *t = tree->txt_root;
	struct text_node n, *l, *r, *y;
	size_t parental_left_offset = 0;

	if (t == NULL)
		return;

	/*check_tree_invariants(tree->txt_root);*/

	n.txn_left = n.txn_right = NULL;
	n.txn_left_size = n.txn_right_size = 0;
	l = r = &n;

	breakpoint();

	for (;;) {
		/* we cannot assume that t is always the root: sometimes it
		 * isn't, sometimes it's the left or right child of the last
		 * thing that was t
		size_t le = left_edge(t);
		*/
		size_t le = parental_left_offset + left_edge(t);
		size_t re = parental_left_offset + right_edge(t);
		if (cursor <= le && !(cursor == 0 && le == 0)) {
			if (t->txn_left == NULL)
				break;
			if (cursor <= parental_left_offset + left_edge(t->txn_left)) {
				y = t->txn_left;
				t->txn_left = y->txn_right;
				t->txn_left_size = y->txn_right_size;
				if (t->txn_left)
					t->txn_left->txn_parent = t;
				y->txn_right = t;
				y->txn_right_size += t->txn_right_size + t->txn_piece->txp_len;
				t->txn_parent = y;
				t = y;
				if (t->txn_left == NULL)
					break;
			}
			r->txn_left = t;
			r->txn_left_size = calc_size(t);
			if (r != &n)
				t->txn_parent = r;
			r = t;
			t = t->txn_left;
		} else if (cursor > re) {
			if (t->txn_right == NULL)
				break;
			if (cursor > re + right_edge(t->txn_right)) {
				y = t->txn_right;
				t->txn_right = y->txn_left;
				t->txn_right_size = y->txn_left_size;
				if (t->txn_right)
					t->txn_right->txn_parent = t;
				y->txn_left = t;
				y->txn_left_size += t->txn_left_size + t->txn_piece->txp_len;
				t->txn_parent = y;
				t = y;
				if (t->txn_right == NULL)
					break;
			}
			l->txn_right = t;
			l->txn_right_size = calc_size(t);
			if (l != &n)
				t->txn_parent = l;
			l = t;
			parental_left_offset += right_edge(t);
			t = t->txn_right;
		} else break;
	}

	l->txn_right = t->txn_left;
	l->txn_right_size = t->txn_left_size;
	r->txn_left = t->txn_right;
	r->txn_left_size = t->txn_right_size;

	if (l->txn_right)
		l->txn_right->txn_parent = l;
	if (r->txn_left)
		r->txn_left->txn_parent = r;

	{
		size_t left_total = t->txn_piece->txp_len + t->txn_left_size;
		size_t right_total = t->txn_piece->txp_len + t->txn_right_size;
		y = &n;
		while (y != r) {
			/*y->txn_left_size -= left_total;*/
			y->txn_left_size = true_size(y->txn_left);
			y = y->txn_left;
		}
		y = &n;
		while (y != l) {
			/*y->txn_right_size -= right_total;*/
			y->txn_right_size = true_size(y->txn_right);
			y = y->txn_right;
		}
	}

	t->txn_left = n.txn_right;
	t->txn_right = n.txn_left;
	t->txn_left_size = calc_size(t->txn_left);
	t->txn_right_size = calc_size(t->txn_right);

	if (t->txn_left)
		t->txn_left->txn_parent = t;
	if (t->txn_right)
		t->txn_right->txn_parent = t;

	t->txn_parent = NULL;
	check_tree_invariants(tree->txt_root);
	tree->txt_root = t;
}

static
void
text_tree_init(struct text_tree *tree)
{
	tree->txt_root = NULL;
	tree->txt_alloc = NULL;
}

static
void
text_tree_finish(struct text_tree *tree)
{
	(void)tree;
}

/* parent is the length of everything preceding this node that is above
 * it in the tree */
static
void
print_text_tree_inorder(struct text_node *t, size_t cursor, size_t parent)
{
	size_t start, end, len;

	if (t == NULL)
		return;

	print_text_tree_inorder(t->txn_left, cursor, parent);

	start = parent + t->txn_left_size;
	len = t->txn_piece->txp_len;
	end = start + len;

	if (cursor > start && cursor <= end) {
		size_t offset = cursor - start;
		eprintf("%.*s", (int)(offset), t->txn_piece->txp_buf);
		eprintf("|");
		eprintf("%.*s", (int)(len - offset), t->txn_piece->txp_buf + offset);
	} else {
		eprintf("%.*s", (int)len, t->txn_piece->txp_buf);
	}

	print_text_tree_inorder(t->txn_right, cursor, end);
}

/* parent is the length of everything preceding this node that is above
 * it in the tree */
static
void
copy_to_buffer(struct text_node *t, size_t first, size_t last, char *buf, size_t cap, size_t *copied)
{
	size_t subcopied;
	int do_before = 0, do_after = 0;
	size_t start, len, end, offset, amount, front, back;
	*copied = 0;

	if (t == NULL)
		return;

	/* the first start and end positions and length of this node's piece */
	start = t->txn_left_size;
	len = t->txn_piece->txp_len;
	end = start + len;

	efprintf(stderr, "first=%lu, last=%lu, start=%lu, end=%lu, cap=%lu\n",
		first, last, start, end, cap);

	if (end <= first) {
		efprintf(stderr, "going right");
		return copy_to_buffer(t->txn_right, first, last, buf, cap, copied);
	}
	if (last <= start) {
		efprintf(stderr, "going left");
		return copy_to_buffer(t->txn_left, first, last, buf, cap, copied);
	}

	/*
	copy_to_buffer(t->txn_left, first, last, buf, cap, &subcopied);
	cap -= subcopied;
	buf += subcopied;
	*copied += subcopied;

	copy_to_buffer(t->txn_right, first, last, buf, cap, &subcopied);
	cap -= subcopied;
	buf += subcopied;
	*copied += subcopied;
	*/

	if (start <= first && first <= end && end <= last) {
		front = first;
		back = end;
		do_after = 1;
	} else if (start <= first && last <= end) {
		front = first;
		back = last;
	} else if (first <= start && end <= last) {
		front = start;
		back = end;
		do_before = 1;
		do_after = 1;
	} else if (first <= start && start <= last && last <= end) {
		front = start;
		back = last;
		do_before = 1;
	}

	if (do_before) {
		efprintf(stderr, "do_before!\n");
		copy_to_buffer(t->txn_left, first, last, buf, cap, &subcopied);
		cap -= subcopied;
		buf += subcopied;
		*copied += subcopied;
	}

	offset = front - start;
	amount = back - front;
	if (amount > cap) amount = cap;

	strncpy(buf, t->txn_piece->txp_buf + offset, amount);
	cap -= amount;
	buf += amount;
	*copied += amount;

	if (do_after) {
		efprintf(stderr, "do_after!\n");
		copy_to_buffer(t->txn_right, first, last, buf, cap, &subcopied);
		cap -= subcopied;
		buf += subcopied;
		*copied += subcopied;
	}
}
