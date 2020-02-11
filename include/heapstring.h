#ifdef VINE_HEAPSTRING_H_INCLUDED
#error "May not include heapstring.h more than once"
#endif
#define VINE_HEAPSTRING_H_INCLUDED
struct heapstring {
	size_t hs_cap;
	char   hs_str[1];
};
extern struct heapstring *heapstring_create(size_t);
extern struct heapstring *heapstring_expand(struct heapstring *, size_t);
extern void heapstring_destroy(struct heapstring *);
