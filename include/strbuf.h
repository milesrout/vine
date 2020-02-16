#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including strbuf.h"
#endif
#ifdef VINE_STRBUF_H_INCLUDED
#error "May not include strbuf.h more than once"
#endif
#define VINE_STRBUF_H_INCLUDED
struct strbuf {
	size_t        sb_len;
	size_t        sb_cap;
	char         *sb_str;
	struct alloc *sb_alloc;
};
extern void strbuf_init(struct strbuf *);
extern void strbuf_init_with(struct strbuf *, struct alloc *, size_t);
extern void strbuf_finish(struct strbuf *);
extern void strbuf_expand_to(struct strbuf *, size_t);
extern void strbuf_expand_by(struct strbuf *, size_t);
extern void strbuf_append_char(struct strbuf *, char);
extern void strbuf_append_cstring(struct strbuf *, const char *);
extern void strbuf_shrink_to_fit(struct strbuf *);
extern const char *strbuf_as_cstring(struct strbuf *);
