#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <termios.h>

#include "abort.h"
#include "alloc.h"
#include "checked.h"
#include "types.h"
#include "printf.h"
#include "memory.h"
#include "text.h"

static void text_tree_init(struct text_tree *);
static void text_tree_finish(struct text_tree *);
static void text_tree_insert_before(struct text_tree *, struct text_node *);
static void text_tree_insert_after(struct text_tree *, struct text_node *);
static void text_tree_replace(struct text_tree *, struct text_node *);
static void text_tree_split(struct text *, size_t);
static struct text_node *text_tree_get(struct text_tree *, size_t);
static struct text_node *get_active_piece(struct text *, size_t);
static void do_splay(struct text_tree *, size_t);
static void print_text_tree_inorder(struct text_node *, size_t, size_t);

void
text_init(struct text *tx, struct alloc *alloc, char *buf, size_t len)
{
	tx->tx_alloc = alloc;

	/* this needs to be a linked list, right? */
	tx->tx_pieces = allocarray_with(alloc, sizeof(struct text_piece), 1);
	tx->tx_pieces[0].txp_buf = buf;
	tx->tx_pieces[0].txp_len = len;
	tx->tx_pieces_len = 1;
	tx->tx_pieces_cap = 1;
	tx->tx_active_piece = NULL;

	text_tree_init(&tx->tx_tree);
	tx->tx_tree.txt_root = allocate_with(alloc, sizeof(struct text_node));
	tx->tx_tree.txt_root->txn_parent = NULL;
	tx->tx_tree.txt_root->txn_left = NULL;
	tx->tx_tree.txt_root->txn_left_size = 0;
	tx->tx_tree.txt_root->txn_right = NULL;
	tx->tx_tree.txt_root->txn_right_size = 0;
	tx->tx_tree.txt_root->txn_piece = tx->tx_pieces;

	tx->tx_orig_buf = buf;
	tx->tx_orig_buf_len = len;

	/* this needs to be a linked list */
	tx->tx_append_buf = allocate_with(&mmap_alloc, PAGE_SIZE);
	tx->tx_append_buf_len = 0;
	tx->tx_append_buf_cap = PAGE_SIZE;
}

void
text_finish(struct text *tx)
{
	deallocarray_with(tx->tx_alloc, tx->tx_pieces,
		sizeof(struct text_piece), tx->tx_pieces_cap);
	text_tree_finish(&tx->tx_tree);

	deallocate_with(&mmap_alloc, tx->tx_append_buf, tx->tx_append_buf_cap);
}

static
struct text_piece *
uninit_text_piece(struct text *tx)
{
	if (tx->tx_pieces_len == tx->tx_pieces_cap) {
		tx->tx_pieces = reallocarray_with(tx->tx_alloc,
			tx->tx_pieces,
			sizeof(struct text_piece),
			tx->tx_pieces_cap,
			tx->tx_pieces_cap * 2);
	}

	return tx->tx_pieces + tx->tx_pieces_len++;
}

void
test_text(void)
{
	int insert = 0;
	struct text tx;
	char buf[] = "Hello, world!";
	size_t cursor = 0;
	size_t size = sizeof(buf) - 1;
	static struct termios oldterm, newterm;

	tcgetattr(STDIN_FILENO, &oldterm);
	newterm = oldterm;
	newterm.c_lflag &= (unsigned)~(ICANON);
	tcsetattr(STDIN_FILENO, TCSANOW, &newterm);

	text_init(&tx, &mmap_alloc, buf, sizeof(buf) - 1);
	eprintf("%.*s|%s\n", (int)cursor, buf, buf + cursor);
	for (;;) {
		int c;
		c = getchar();
		if (insert) {
			if ('A' <= c && c <= 'Z') {
				struct text_node *n = get_active_piece(&tx, cursor);
				tx.tx_append_buf[tx.tx_append_buf_len++] = (char)c;
				n->txn_piece->txp_len++;
				cursor++;
				size++;
			} else {
				/* commit active piece */
				tx.tx_active_piece = NULL;
				tx.tx_append_buf[tx.tx_append_buf_len++] = '\0';
				if (tx.tx_append_buf_len > 20)
					breakpoint();
				insert = 0;
			}
		} else {
			if (c == EOF || c == 'q')
				break;
			else if (c == 'l')
				cursor = (cursor + 1) % size;
			else if (c == 'h')
				cursor = (cursor - 1) % size;
			else if (c == 'i') {
				insert = 1;
			}
		}
		print_text_tree_inorder(tx.tx_tree.txt_root, cursor, 0);
		eprintf("\n");
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
	text_finish(&tx);
}

/* Return a struct text_piece at the cursor that we are allowed to append to
 */
static
struct text_node *
get_active_piece(struct text *tx, size_t cursor)
{
	struct text_node *n;

	/* splay the node corresponding to the current cursor position */
	n = text_tree_get(&tx->tx_tree, cursor);

	/* if the cursor points to the end of the currently active piece */
	if (n->txn_piece == tx->tx_active_piece)
		if (cursor == n->txn_left_size + n->txn_piece->txp_len)
			return n;

	/* otherwise, we need to create a new editable piece and make it the
	 * current piece */
	text_tree_split(tx, cursor);

	tx->tx_active_piece = tx->tx_tree.txt_root->txn_piece;
	return tx->tx_tree.txt_root;
}

/* Split the given node at the position 'cursor'.  Requires that cursor points
 * into, immediately after or immediately before the node to split.  Requires
 * that the node to split is at the root of the tree.
 */
static
void
text_tree_split(struct text *tx, size_t cursor)
{
	struct text_node *n = tx->tx_tree.txt_root;

	/* because n is the root, its 'left size' is the size of all the text
	 * on the left of this node i.e. the offset of the text in this node
	 * from the beginning of the document */
	size_t left_edge_pos = n->txn_left_size;

	/* same logic */
	size_t right_edge_pos = n->txn_left_size + n->txn_piece->txp_len;

	/* we always need to create a new blank and editable text piece */
	struct text_node *P = allocate_with(tx->tx_alloc, sizeof(struct text_node));
	struct text_piece *p = uninit_text_piece(tx);
	p->txp_buf = tx->tx_append_buf + tx->tx_append_buf_len;
	p->txp_len = 0;

	/* it is zero-initialised by allocate_with and as such it should not be
	 * necessary to initialise things to NULL/0 */
	P->txn_parent = NULL;
	P->txn_left = P->txn_right = NULL;
	P->txn_left_size = P->txn_right_size = 0;
	P->txn_piece = p;

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
		text_tree_insert_before(&tx->tx_tree, P);
	} else if (cursor == right_edge_pos) {
		/* the cursor points directly after n.  we want to insert the
		 * new node directly after n. */
		text_tree_insert_after(&tx->tx_tree, P);
	} else if (cursor < left_edge_pos || cursor > right_edge_pos) {
		breakpoint();
	} else {
		/* the trickier case.  the cursor points within n.  we want to
		 * create two new nodes corresponding to everything in
		 * n before the cursor and everything in n after the
		 * cursor. then we want to insert the three new nodes, making
		 * sure to insert the new blank and editable node last (or at
		 * least that it is the root at the end - insert all three at
		 * once and then splay it up perhaps? */
		struct text_piece *l = uninit_text_piece(tx);
		struct text_piece *r = uninit_text_piece(tx);
		struct text_node *L = allocate_with(tx->tx_alloc, sizeof(struct text_node));
		struct text_node *R = allocate_with(tx->tx_alloc, sizeof(struct text_node));

		/* how far into the node the cursor is */
		size_t offset = cursor - left_edge_pos;

		l->txp_buf = n->txn_piece->txp_buf;
		l->txp_len = offset;
		r->txp_buf = n->txn_piece->txp_buf + offset;
		r->txp_len = n->txn_piece->txp_len - offset;

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

/* Add a new childless node immediately before the root node in the tree */
static
void
text_tree_insert_before(struct text_tree *tree, struct text_node *new)
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
text_tree_insert_after(struct text_tree *tree, struct text_node *new)
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
 * children, each of which must have exactly zero children. */
static
void
text_tree_replace(struct text_tree *tree, struct text_node *new)
{
	new->txn_left->txn_left = tree->txt_root->txn_left;
	new->txn_left->txn_left_size = tree->txt_root->txn_left_size;
	new->txn_left_size = new->txn_left->txn_left_size + new->txn_left->txn_piece->txp_len;

	new->txn_right->txn_right = tree->txt_root->txn_right;
	new->txn_right->txn_right_size = tree->txt_root->txn_right_size;
	new->txn_right_size = new->txn_right->txn_right_size + new->txn_right->txn_piece->txp_len;

	if (new->txn_left->txn_left)
		new->txn_left->txn_left->txn_parent = new->txn_left;
	if (new->txn_right->txn_right)
		new->txn_right->txn_right->txn_parent = new->txn_right;

	tree->txt_root = new;
}

static
struct text_node *
text_tree_get(struct text_tree *tree, size_t cursor)
{
	do_splay(tree, cursor);
	return tree->txt_root;
}

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
		size_t result = 0;
		breakpoint();
		return result;
	}
}

static
size_t
left_edge(struct text_node *n)
{
	return parental_left_offset(n) + n->txn_left_size;
}

static
size_t
right_edge(struct text_node *n)
{
	return left_edge(n) + n->txn_piece->txp_len;
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

	if (t == NULL)
		return;

	n.txn_left = n.txn_right = NULL;
	n.txn_left_size = n.txn_right_size = 0;
	l = r = &n;

	for (;;) {
		/* we cannot assume that t is always the root: sometimes it
		 * isn't, sometimes it's the left or right child of the last
		 * thing that was t */
		size_t le = left_edge(t);
		if (cursor <= le && !(cursor == 0 && le == 0)) {
			if (t->txn_left == NULL)
				break;
			if (cursor <= left_edge(t->txn_left)) {
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
			r = t;
			t->txn_parent = NULL;
			t = t->txn_left;
		} else if (cursor > right_edge(t)) {
			if (t->txn_right == NULL)
				break;
			if (cursor > right_edge(t->txn_right)) {
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
			l = t;
			t->txn_parent = NULL;
			t = t->txn_right;
		} else break;
	}

	l->txn_right = t->txn_left;
	l->txn_right_size = t->txn_left_size;
	r->txn_left = t->txn_right;
	r->txn_left_size = t->txn_right_size;

	{
		size_t left_total = t->txn_piece->txp_len + t->txn_left_size;
		size_t right_total = t->txn_piece->txp_len + t->txn_right_size;
		y = &n;
		while (y != r) {
			y->txn_left_size -= right_total;
			y = y->txn_left;
		}
		y = &n;
		while (y != l) {
			y->txn_right_size -= left_total;
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
