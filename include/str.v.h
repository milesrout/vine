//require alloc.h
//provide str.h
struct strview {
	struct string  *sv_string;
	size_t          sv_len;
	char           *sv_str;
};
struct string {
	size_t          str_refcnt;
	size_t          str_len;
	size_t          str_cap;
	char           *str_str;
	struct alloc   *str_alloc;
	struct alloc   *str_viewalloc;
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
extern struct strview *string_substr(struct string *, size_t, size_t);
extern void strview_destroy(struct strview *);
