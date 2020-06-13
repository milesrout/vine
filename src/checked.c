#include <stddef.h>
#include "abort.h"
#include "checked.h"

int
try_add_sz(size_t l, size_t r, size_t *sum)
{
	*sum = l + r;
	return *sum < l;
}

size_t
add_sz(size_t l, size_t r)
{
	if (l + r < l) {
		abort_with_error("Unsigned overflow of size_t: %lu + %lu", l, r);
	}
	return l + r;
}
