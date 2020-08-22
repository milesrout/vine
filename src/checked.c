#include <stddef.h>
#include "abort.h"
#include "checked.h"

#ifdef __GNUC__

int
try_add_sz(size_t l, size_t r, size_t *sum)
{
	return __builtin_add_overflow(l, r, sum);
}

size_t
add_sz(size_t l, size_t r)
{
	size_t sum;

	if (__builtin_add_overflow(l, r, &sum)) {
		abort_with_error("Unsigned add overflow (size_t): %lu + %lu",
			l, r);
	}

	return sum;
}

int
try_mul_sz(size_t l, size_t r, size_t *sum)
{
	return __builtin_mul_overflow(l, r, sum);
}

size_t
mul_sz(size_t l, size_t r)
{
	size_t product;

	if (__builtin_mul_overflow(l, r, &product)) {
		abort_with_error("Unsigned mul overflow (size_t): %lu + %lu",
			l, r);
	}

	return product;
}


#else
#error "Checked arithmetic not yet implemented for your platform"
#endif

/*
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
		abort_with_error("Unsigned overflow (size_t): %lu + %lu", l, r);
	}
	return l + r;
}
*/
