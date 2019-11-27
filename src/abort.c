#include "abort.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void abort_with_error(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	/* The return value of this function doesn't really matter. If it
	 * fails, it fails. If it doesn't, we got some sort of error message
	 * out at least. Either way we're going to abort.
	 */
	(void)vfprintf(stderr, fmt, args);

	va_end(args);

	abort();
}
