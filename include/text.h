#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including text.h"
#endif
#ifdef VINE_TEXT_H_INCLUDED
#error "May not include text.h more than once"
#endif
#define VINE_TEXT_H_INCLUDED
struct text_piece {
	char  *txp_buf;
	size_t txp_len;
};
struct text_tree {
	struct text_node  *txt_root;
	struct alloc      *txt_alloc;
};
struct text_node {
	struct text_node  *txn_parent;
	struct text_node  *txn_left;
	size_t             txn_left_size;
	struct text_node  *txn_right;
	size_t             txn_right_size;
	struct text_piece *txn_piece;
};
struct text {
	struct alloc      *tx_alloc;
	struct text_piece *tx_pieces;
	struct text_piece *tx_active_piece;
	size_t             tx_pieces_len;
	size_t             tx_pieces_cap;
	struct text_tree   tx_tree;
	const char        *tx_orig_buf;
	size_t             tx_orig_buf_len;
	char              *tx_append_buf;
	size_t             tx_append_buf_len;
	size_t             tx_append_buf_cap;
};
extern void text_init(struct text *, struct alloc *, char *, size_t);
extern void text_finish(struct text *);
extern void test_text(void);
