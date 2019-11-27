#include <stdlib.h>
#include "_memory.h"

#include "abort.h"

void *emalloc(size_t n)
{
	void *p = malloc(n);
	if (p == NULL)
		abort_with_error("Memory allocation failure\n");

	return p;
}

void *ecalloc(size_t n, size_t m)
{
	void *p = calloc(n, m);
	if (p == NULL)
		abort_with_error("Memory allocation failure\n");

	return p;
}

void *erealloc(void *q, size_t n)
{
	void *p = realloc(q, n);
	if (p == NULL)
		abort_with_error("Memory allocation failure\n");

	return p;
}
