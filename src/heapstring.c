#include <stddef.h>
#include "abort.h"
#include "memory.h"
#include "heapstring.h"

#define STRING_MIN_CAP 16

struct heapstring *
heapstring_create(size_t cap)
{
	struct heapstring *str;

	if (cap < STRING_MIN_CAP) {
		cap = STRING_MIN_CAP;
	}

	str = allocate(sizeof *str + cap - 1);
	str->hs_cap = cap;

	return str;
}

struct heapstring *
heapstring_expand(struct heapstring *str, size_t newcap)
{
	(void)str;
	(void)newcap;
	abort_with_error("Unimplemented!");
	return NULL;
}

void
heapstring_destroy(struct heapstring *str)
{
	deallocate(str, sizeof *str + str->hs_cap - 1);
}
