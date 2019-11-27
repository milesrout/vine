#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "printf.h"
#include "memory.h"

int main()
{
	void *x;

	efprintf(stdout, "Hello, world!\n");
	x = emalloc(sizeof(int));
	free(x);

	return 0;
}
