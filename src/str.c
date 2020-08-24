#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "abort.h"
#include "alloc.h"
#include "checked.h"
#include "str.h"
#include "util.h"
#include "printf.h"

#define STRING_MIN_CAP 16

static void
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
	str->str_node.prev = NULL;
	str->str_node.next = NULL;
}

static struct string *
str_node_parent(struct str_node *node)
{
	while (node->prev != NULL) {
		node = node->prev;
	}
	return container_of(struct string, str_node, node);
}

void
string_init(struct string *str)
{
	str->str_len = 0;
	str->str_cap = 0;
	str->str_str = NULL;
	str->str_alloc = &sys_alloc;
	str->str_viewalloc = str->str_alloc;
	str->str_node.prev = NULL;
	str->str_node.next = NULL;
	str->str_finished = 0;
}

void
string_init_with(struct string *str, struct alloc *alloc, size_t cap)
{
	str->str_len = 0;
	str->str_cap = cap;
	if (cap == 0) {
		str->str_str = NULL;
	} else {
		str->str_str = allocate_with(alloc, cap);
	}
	str->str_alloc = alloc;
	str->str_viewalloc = str->str_alloc;
	str->str_node.prev = NULL;
	str->str_node.next = NULL;
	str->str_finished = 0;
}

void
string_finish(struct string *str)
{
	str->str_finished = 1;
	if (str->str_node.next == NULL) {
		destroy(str);
	}
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

	if (str->str_node.next != NULL) {
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

	eprintf("old capacity %lu\n", str->str_cap);
	eprintf("old length %lu\n", str->str_len);

	if (str->str_len + len >= str->str_cap) {
		string_expand_by(str, len);
	}

	eprintf("new capacity %lu\n", str->str_cap);
	eprintf("cstr length %lu\n", len);

	strncpy(str->str_str + str->str_len, cstr, len);
	str->str_len += len;

	eprintf("new length %lu\n", str->str_len);
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
	struct str_node sv_node;
	struct strview *sv = allocate_with(str->str_viewalloc, sizeof *sv);

	sv_node.prev = &str->str_node;
	sv_node.next = str->str_node.next;

	sv->sv_len = str->str_len;
	sv->sv_str = str->str_str;
	sv->sv_node = sv_node;

	if (str->str_node.next != NULL) {
		str->str_node.next->prev = &sv->sv_node;
	}
	str->str_node.next = &sv->sv_node;

	return sv;
}

struct strview *
string_substr(struct string *str, size_t start, size_t len)
{
	struct str_node sv_node;
	struct strview *sv = allocate_with(str->str_viewalloc, sizeof *sv);

	sv_node.prev = &str->str_node;
	sv_node.next = str->str_node.next;

	sv->sv_len = len;
	sv->sv_str = str->str_str + start;
	sv->sv_node = sv_node;

	if (str->str_node.next != NULL) {
		str->str_node.next->prev = &sv->sv_node;
	}
	str->str_node.next = &sv->sv_node;

	return sv;
}

void strview_destroy(struct strview *sv)
{
	struct string *str = str_node_parent(&sv->sv_node);
	struct alloc *alloc = str->str_viewalloc;

	sv->sv_len = 0;
	sv->sv_str = NULL;

	if (sv->sv_node.next == NULL && sv->sv_node.prev->prev == NULL) {
		if (str->str_finished) {
			destroy(str);
		} else {
			str->str_node.next = NULL;
		}
	} else {
		sv->sv_node.prev->next = sv->sv_node.next;
		if (sv->sv_node.next != NULL) {
			sv->sv_node.next->prev = sv->sv_node.prev;
		}
	}
	deallocate_with(alloc, sv, sizeof *sv);
}
