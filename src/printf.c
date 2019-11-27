#include <stdarg.h>
#include <stdio.h>
#include "abort.h"
#include "_printf.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

void evfprintf(FILE *file, const char *fmt, va_list args)
{
	int err;
	err = vfprintf(file, fmt, args);
	if (err < 0) {
		/* Try to print to stderr, which might still be working even if
		 * the other stream isn't.
		 */
		abort_with_error("efprintf failed: %s\n", strerror(errno));
	}
}

void efprintf(FILE *file, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	evfprintf(file, fmt, args);
	va_end(args);
}

void eprintf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	evfprintf(stdout, fmt, args);
	va_end(args);
}
