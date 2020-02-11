#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including strbuf.h"
#endif
#ifdef VINE_STRBUF_H_INCLUDED
#error "May not include strbuf.h more than once"
#endif
#define VINE_STRBUF_H_INCLUDED
/* There are several different types of string we deal with in vine.
 *
 * The simplest are immutable C strings: a pointer to a null-terminated,
 * immutable array of characters. These are the strings that the standard C
 * library is designed to deal with. These must never be modified, for doing so
 * is undefined behaviour. In practice, these can and are deduplicated and
 * merged so that two different string literals point into overlapping strings,
 * so modifying one could modify another. The compiler will also assume you do
 * not do so, and could assume that any code that does do so is unreachable and
 * elide it.
 *
 * Next there is the humble C string: a pointer to a null-terminated array of
 * characters, which is *not* necessarily immutable, even if it is being
 * accessed through a pointer-to-const-char. These are notoriously error-prone
 * and should be avoided at all costs. When they are necessary for interfacing
 * with third-party APIs they should be converted to a reasonable string
 * representation if at all possible. It should be unusual for a third-party
 * API to force us to deal with mutable char* strings in-place, though, so they
 * shouldn't be very common.
 *
 * Next we have fixed-length, length-prefixed, heap-allocated strings:
 * 'struct string *' and its friends. These should nearly always be immutable,
 * because there aren't many useful string operations that are
 * length-preserving (in a Unicode world even substituting one character for
 * another is not length-preserving), and they cannot be resized in place.
 *
 * We also have strings in the style of C++'s 'std::string', consisting of a
 * length and a pointer to a heap-allocated array of characters that can be
 * reallocated when the string needs to be resized.
 *
 * Then there are ropes. Ropes are most suitable for very large strings that
 * have to change a lot.
 */
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