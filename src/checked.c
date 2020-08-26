#include <stddef.h>
#include <stdint.h>

#include "abort.h"
#include "checked.h"
#include "types.h"

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

int
try_add_uip(uintptr_t l, size_t r, uintptr_t *sum)
{
	return __builtin_add_overflow(l, r, sum);
}

uintptr_t
add_uip(uintptr_t l, size_t r)
{
	uintptr_t sum;

	if (__builtin_add_overflow(l, r, &sum)) {
		abort_with_error("Unsigned add overflow (size_t): %lu + %lu",
			l, r);
	}

	return sum;
}


#else
#error "Checked arithmetic not yet implemented for your platform"
#endif

size_t
align_sz(size_t s, size_t align)
{
	if (align == 0)
		return s;
	return add_sz(s, (size_t)(align - 1)) & ~((size_t)(align - 1));
}

char *
align_ptr(char *ptr, size_t align)
{
	uintptr_t iptr = (uintptr_t)ptr;

	if (align == 0)
		return ptr;

	return (char *)(add_uip(iptr, (align - 1)) & ~(align - 1));
}
