#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including str.h"
#endif
#ifdef VINE_STR_H_INCLUDED
#error "May not include str.h more than once"
#endif
#define VINE_STR_H_INCLUDED
struct str_node {
	struct str_node *prev;
	struct str_node *next;
};
struct strview {
	struct str_node sv_node;
	size_t          sv_len;
	char           *sv_str;
};
struct string {
	struct str_node str_node;
	size_t          str_len;
	size_t          str_cap;
	char           *str_str;
	struct alloc   *str_alloc;
	int             str_finished;
};
extern void string_init(struct string *);
extern void string_init_with(struct string *, struct alloc *, size_t);
extern void string_finish(struct string *);
extern void string_expand_to(struct string *, size_t);
extern void string_expand_by(struct string *, size_t);
extern void string_append_char(struct string *, char);
extern void string_append_cstring(struct string *, const char *);
extern void string_shrink_to_fit(struct string *);
extern struct strview *string_as_view(struct string *);
extern void strview_destroy(struct strview *);
