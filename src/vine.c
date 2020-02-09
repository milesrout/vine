#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printf.h"
#include "str.h"
#include "memory.h"
#include "alloc.h"
#include "alloc_buf.h"

int main()
{

	{
		int *x;
		x = allocate(sizeof(int));
		efprintf(stdout, "%llu bytes at %p with default allocator\n", sizeof(int), x);
		deallocate(x, sizeof(int));
		x = allocate(sizeof(int));
		efprintf(stdout, "%llu bytes at %p with default allocator\n", sizeof(int), x);
		deallocate(x, sizeof(int));
		x = allocate(sizeof(int));
		efprintf(stdout, "%llu bytes at %p with default allocator\n", sizeof(int), x);
		deallocate(x, sizeof(int));
	}

	{
		int *x;
		char buf[256];
		struct buf_alloc ba;

		buf_alloc_init(&ba, buf, 256);

		x = allocate_with(&ba.ba_alloc, sizeof(int));
		efprintf(stdout, "%llu bytes at %p with buffer allocator\n", sizeof(int), x);
		deallocate_with(&ba.ba_alloc, x, sizeof(int));
		x = allocate_with(&ba.ba_alloc, sizeof(int));
		efprintf(stdout, "%llu bytes at %p with buffer allocator\n", sizeof(int), x);
		deallocate_with(&ba.ba_alloc, x, sizeof(int));
		x = allocate_with(&ba.ba_alloc, sizeof(int));
		efprintf(stdout, "%llu bytes at %p with buffer allocator\n", sizeof(int), x);
		deallocate_with(&ba.ba_alloc, x, sizeof(int));
	}

	{
		struct strbuf sb1;
		strbuf_init(&sb1);
		strbuf_append_cstring(&sb1, "Miles");
		strbuf_append_char(&sb1, '!');
		efprintf(stdout, "Hello, %s\n", strbuf_as_cstring(&sb1));
		strbuf_finish(&sb1);
	}

	{
		struct string *str = string_create(14);
		strcpy(str->str_str, "Hello, world!");
		efprintf(stdout, "%s\n", str->str_str);
		string_destroy(str);
	}

	return 0;
}
