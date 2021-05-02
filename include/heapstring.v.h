//require alloc.h
//provide heapstring.h
struct heapstring {
	size_t hs_cap;
	char   hs_str[1];
};
extern struct heapstring *heapstring_create(size_t, struct alloc *);
extern struct heapstring *heapstring_expand(struct heapstring *, struct alloc *, size_t);
extern void heapstring_destroy(struct heapstring *, struct alloc *);
#define HEAPSTRING_PAGE_CAP (PAGE_SIZE - sizeof(struct heapstring) + 1)
