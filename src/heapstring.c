#include <stddef.h>

#include "abort.h"
#include "alloc.h"
#include "heapstring.h"

#define STRING_MIN_CAP 16

struct heapstring *
heapstring_create(size_t cap, struct alloc *a)
{
	struct heapstring *str;

	if (cap < STRING_MIN_CAP) {
		cap = STRING_MIN_CAP;
	}

	str = allocate_with(a, sizeof *str + cap - 1);
	str->hs_cap = cap;

	return str;
}

struct heapstring *
heapstring_expand(struct heapstring *str, struct alloc *a, size_t newcap)
{
	return reallocate_with(a, str,
		sizeof *str + str->hs_cap - 1,
		sizeof *str + newcap - 1);
}

void
heapstring_destroy(struct heapstring *str, struct alloc *a)
{
	deallocate_with(a, str, sizeof *str + str->hs_cap - 1);
}
