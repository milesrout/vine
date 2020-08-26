#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abort.h"
#include "alloc.h"
#include "strbuf.h"
#include "printf.h"
#include "memory.h"
#include "checked.h"

#define STRBUF_MIN_CAP 16

static
void
assert_strbuf_invariant(struct strbuf *sb)
{
#ifndef NDEBUG
	if (sb->sb_str == NULL) {
		assert1(sb->sb_len == 0);
		assert1(sb->sb_cap == 0);
	} else {
		assert1(sb->sb_cap >= STRBUF_MIN_CAP);
	}
	assert1(sb->sb_len <= sb->sb_cap);
#endif
}

void
strbuf_init(struct strbuf *sb)
{
	sb->sb_len = 0;
	sb->sb_cap = 0;
	sb->sb_str = NULL;
	sb->sb_alloc = &sys_alloc;
}

void
strbuf_init_with(struct strbuf *sb, struct alloc *alloc, size_t cap)
{
	sb->sb_len = 0;
	sb->sb_cap = cap;
	if (cap == 0) {
		sb->sb_str = NULL;
	} else {
		sb->sb_str = allocate_with(alloc, cap);
	}
	sb->sb_alloc = alloc;
	assert_strbuf_invariant(sb);
}

void
strbuf_finish(struct strbuf *sb)
{
	assert_strbuf_invariant(sb);
	sb->sb_len = 0;
	if (sb->sb_str != NULL) {
		deallocate_with(sb->sb_alloc, sb->sb_str, sb->sb_cap);
		sb->sb_str = NULL;
	}
	sb->sb_cap = 0;
	sb->sb_alloc = NULL;
}

void
strbuf_expand_to(struct strbuf *sb, size_t atleast)
{
	assert_strbuf_invariant(sb);
	if (sb->sb_cap >= atleast) {
		return;
	}

	if (atleast < STRBUF_MIN_CAP) {
		atleast = STRBUF_MIN_CAP;
	}

	if (sb->sb_str == NULL) {
		sb->sb_str = allocate_with(sb->sb_alloc, atleast);
	} else {
		sb->sb_str = reallocate_with(
			sb->sb_alloc,
			sb->sb_str,
			sb->sb_cap,
			atleast);
	}
	sb->sb_cap = atleast;
	assert_strbuf_invariant(sb);
}

void
strbuf_expand_by(struct strbuf *sb, size_t atleast)
{
	size_t newcap;
	if (atleast < sb->sb_cap) {
		newcap = sb->sb_cap * 2; /* add_sz(sb->sb_cap, sb->sb_cap);*/
	} else {
		newcap = sb->sb_cap + atleast; /* add_sz(sb->sb_cap, atleast); */
	}
	strbuf_expand_to(sb, newcap);
}

void
strbuf_append_char(struct strbuf *sb, char ch)
{
	assert_strbuf_invariant(sb);

	/* technically speaking, if str==NULL then len==cap==0.  however it is
	 * better to be safe than to be sorry. */
	if (sb->sb_len == sb->sb_cap) {
		strbuf_expand_by(sb, 1);
	}


	assert_strbuf_invariant(sb);

	sb->sb_str[sb->sb_len] = ch;
	sb->sb_len += 1;
}

void
strbuf_append_cstring(struct strbuf *sb, const char *cstr)
{
	size_t len = strlen(cstr);

	if (sb->sb_len + len >= sb->sb_cap) {
		strbuf_expand_by(sb, len);
	}

	assert_strbuf_invariant(sb);

	strncpy(sb->sb_str + sb->sb_len, cstr, len);
	sb->sb_len += len;
}

void
strbuf_shrink_to_fit(struct strbuf *sb)
{
	assert_strbuf_invariant(sb);

	if (sb->sb_len == sb->sb_cap) {
		return;
	}

	sb->sb_str = reallocate_with(sb->sb_alloc, sb->sb_str, sb->sb_cap, sb->sb_len);
	sb->sb_cap = sb->sb_len;

	assert_strbuf_invariant(sb);
}

const char *
strbuf_as_cstring(struct strbuf *sb)
{
	if (sb->sb_str == NULL || sb->sb_str[sb->sb_len - 1] != '\0') {
		strbuf_append_char(sb, '\0');
	}
	return sb->sb_str;
}
