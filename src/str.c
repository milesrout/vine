#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "abort.h"
#include "alloc.h"
#include "checked.h"
#include "str.h"
#include "util.h"
#include "printf.h"

#define STRING_MIN_CAP 16

static
void
destroy(struct string *str)
{
	str->str_len = 0;
	if (str->str_str != NULL) {
		deallocate_with(str->str_alloc, str->str_str, str->str_cap);
		str->str_str = NULL;
	}
	str->str_cap = 0;
	str->str_alloc = NULL;
	str->str_viewalloc = NULL;
}

void
string_init(struct string *str)
{
	str->str_refcnt = 1;
	str->str_len = 0;
	str->str_cap = 0;
	str->str_str = NULL;
	str->str_alloc = &sys_alloc;
	str->str_viewalloc = &sys_alloc;
}

void
string_init_with(struct string *str, struct alloc *alloc, size_t cap)
{
	str->str_refcnt = 1;
	str->str_len = 0;
	str->str_cap = cap;
	if (cap == 0) {
		str->str_str = NULL;
	} else {
		str->str_str = allocate_with(alloc, cap);
	}
	str->str_alloc = alloc;
	str->str_viewalloc = alloc;
}

void
string_finish(struct string *str)
{
	str->str_refcnt -= 1;
	if (str->str_refcnt == 0)
		destroy(str);
}

void
string_expand_to(struct string *str, size_t atleast)
{
	if (atleast < str->str_cap) {
		return;
	}

	if (atleast < STRING_MIN_CAP) {
		atleast = STRING_MIN_CAP;
	}

	if (str->str_refcnt > 1) {
		abort_with_error("Cannot expand string when there are active views\n");
	}

	if (str->str_str == NULL) {
		str->str_str = allocate_with(str->str_alloc, atleast);
	} else {
		str->str_str = reallocate_with(
			str->str_alloc,
			str->str_str,
			str->str_cap,
			atleast);
	}
	str->str_cap = atleast;
}

void
string_expand_by(struct string *str, size_t atleast)
{
	size_t newcap;
	if (atleast < str->str_cap) {
		newcap = add_sz(str->str_cap, str->str_cap);
	} else {
		newcap = add_sz(str->str_cap, atleast);
	}
	string_expand_to(str, newcap);
}

void
string_append_char(struct string *str, char ch)
{
	if (str->str_len == str->str_cap) {
		string_expand_by(str, 1);
	}

	str->str_str[str->str_len] = ch;
	str->str_len += 1;
}

void
string_append_cstring(struct string *str, const char *cstr)
{
	size_t len = strlen(cstr);

	if (str->str_len + len >= str->str_cap) {
		string_expand_by(str, len);
	}

	strncpy(str->str_str + str->str_len, cstr, len);
	str->str_len += len;
}

void
string_shrink_to_fit(struct string *str)
{
	if (str->str_len == str->str_cap) {
		return;
	}

	str->str_str = reallocate_with(
		str->str_alloc,
		str->str_str,
		str->str_cap,
		str->str_len);
	str->str_cap = str->str_len;
}

struct strview *
string_as_view(struct string *str)
{
	struct strview *sv = allocate_with(str->str_viewalloc, sizeof *sv);

	sv->sv_len = str->str_len;
	sv->sv_str = str->str_str;
	sv->sv_string = str;

	str->str_refcnt += 1;

	return sv;
}

struct strview *
string_substr(struct string *str, size_t start, size_t len)
{
	struct strview *sv = allocate_with(str->str_viewalloc, sizeof *sv);

	sv->sv_len = len;
	sv->sv_str = str->str_str + start;
	sv->sv_string = str;

	str->str_refcnt += 1;

	return sv;
}

void
strview_destroy(struct strview *sv)
{
	struct string *str = sv->sv_string;
	struct alloc *alloc = str->str_viewalloc;

	str->str_refcnt -= 1;

	if (str->str_refcnt == 0)
		destroy(str);

	deallocate_with(alloc, sv, sizeof *sv);
}
