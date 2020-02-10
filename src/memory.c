#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "_memory.h"

#include "abort.h"
#include "alloc.h"
#include "printf.h"

void *try_allocate(size_t m)
{
	return try_allocate_with(&sys_alloc, m);
}

void *allocate(size_t m)
{
	return allocate_with(&sys_alloc, m);
}

void *try_reallocate(void *q, size_t m, size_t n)
{
	return try_reallocate_with(&sys_alloc, q, m, n);
}

void *reallocate(void *q, size_t m, size_t n)
{
	return reallocate_with(&sys_alloc, q, m, n);
}

void deallocate(void *p, size_t m)
{
	deallocate_with(&sys_alloc, p, m);
}
