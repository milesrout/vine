struct buf_alloc {
	struct alloc ba_alloc;
	char *ba_buf;
	size_t ba_cap;
};
extern void buf_alloc_init(struct buf_alloc *, char *, size_t);
