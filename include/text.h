#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including text.h"
#endif
#ifndef VINE_SLAB_POOL_H_INCLUDED
#error "Must include slab_pool.h before including text.h"
#endif
#ifdef VINE_TEXT_H_INCLUDED
#error "May not include text.h more than once"
#endif
#define VINE_TEXT_H_INCLUDED
struct text_buffer {
	struct text_buffer *txb_next;
	char txb_buf[1];
};
#define TEXT_BUFFER_SIZE (sizeof(struct text_buffer) - 1)
enum text_piece_state { TXP_CONST, TXP_MUTABLE };
struct text_piece {
	char  *txp_buf;
	size_t txp_len;
	int    txp_state;
	int    txp_cap; /* only valid if state == TXP_MUTABLE */
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
	struct alloc       *tx_alloc;
	struct slab_pool    tx_pieces_pool;
	struct slab_pool    tx_nodes_pool;
	struct text_tree    tx_tree;
	const char         *tx_orig_buf;
	size_t              tx_orig_buf_len;
	char               *tx_add_buf;
	size_t              tx_add_buf_cap;
};
extern void text_init(struct text *, struct alloc *, char *, size_t);
extern void text_finish(struct text *);
extern void test_text(void);
