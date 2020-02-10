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
		struct strbuf sb;
		strbuf_init(&sb);
		strbuf_append_cstring(&sb, "Miles");
		strbuf_append_char(&sb, '!');
		efprintf(stdout, "Hello, %s\n", strbuf_as_cstring(&sb));
		strbuf_finish(&sb);
	}

	{
		char buf[1024];
		struct buf_alloc ba;
		struct strbuf sb;

		buf_alloc_init(&ba, buf, 1024);

		strbuf_init_with(&sb, &ba.ba_alloc, 0);
		efprintf(stderr, "1\n");
		strbuf_append_cstring(&sb, "abcdefghijklmnopqrstuvwxyz ");
		efprintf(stderr, "2\n");
		strbuf_append_cstring(&sb, "abcdefghijklmnopqrstuvwxyz ");
		efprintf(stderr, "3\n");
		strbuf_append_cstring(&sb, "abcdefghijklmnopqrstuvwxyz ");
		efprintf(stderr, "4\n");
		strbuf_append_cstring(&sb, "abcdefghijklmnopqrstuvwxyz ");
		efprintf(stderr, "5\n");
		strbuf_append_cstring(&sb, "abcdefghijklmnopqrstuvwxyz ");
		efprintf(stderr, "6\n");
		strbuf_append_cstring(&sb, "abcdefghijklmnopqrstuvwxyz ");
		efprintf(stderr, "7\n");
		strbuf_append_cstring(&sb, "abcdefghijklmnopqrstuvwxyz ");
		efprintf(stderr, "8\n");
		strbuf_append_cstring(&sb, "abcdefghijklmnopqrstuvwxyz ");
		efprintf(stderr, "9\n");
		strbuf_append_cstring(&sb, "abcdefghijklmnopqrstuvwxyz ");
		efprintf(stderr, "10\n");
		strbuf_append_cstring(&sb, "Miles");
		efprintf(stderr, "11\n");
		strbuf_append_char(&sb, '!');
		efprintf(stderr, "12\n");
		efprintf(stdout, "Hello, %s\n", strbuf_as_cstring(&sb));
		strbuf_finish(&sb);
	}

	{
		struct string *str = string_create(14);
		strcpy(str->str_str, "Hello, world!");
		efprintf(stdout, "%s\n", str->str_str);
		string_destroy(str);
	}

	return 0;
}
