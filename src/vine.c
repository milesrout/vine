#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printf.h"
#include "memory.h"
#include "alloc.h"
#include "alloc_buf.h"
#include "str.h"
#include "hash.h"

static const char *
autism = "I'd just like to interject for a moment.  What you're referring to as Linux, \
is in fact, GNU/Linux, or as I've recently taken to calling it, GNU plus Linux. \
Linux is not an operating system unto itself, but rather another free component \
of a fully functioning GNU system made useful by the GNU corelibs, shell \
utilities and vital system components comprising a full OS as defined by POSIX.\n"

"Many computer users run a modified version of the GNU system every day, \
without realizing it.  Through a peculiar turn of events, the version of GNU \
which is widely used today is often called \"Linux\", and many of its users are \
not aware that it is basically the GNU system, developed by the GNU Project.\n"

"There really is a Linux, and these people are using it, but it is just a \
part of the system they use.  Linux is the kernel: the program in the system \
that allocates the machine's resources to the other programs that you run. \
The kernel is an essential part of an operating system, but useless by itself; \
it can only function in the context of a complete operating system.  Linux is \
normally used in combination with the GNU operating system: the whole system \
is basically GNU with Linux added, or GNU/Linux.  All the so-called \"Linux\" \
distributions are really distributions of GNU/Linux.";

int
main(void)
{
	{
		struct string str;
		struct strview *sv, *sv_all;
		uint32_t hash1, hash2, hash3;
		string_init(&str);
		string_expand_to(&str, 4096);
		efprintf(stderr, "%p has length %llu, capacity %llu\n",
			         &str, str.str_len, str.str_cap);
		string_append_cstring(&str, autism);
		efprintf(stderr, "%p has length %llu, capacity %llu\n",
			         &str, str.str_len, str.str_cap);
		sv = string_as_view(&str);
		string_append_cstring(&str, autism);
		efprintf(stderr, "%p has length %llu, capacity %llu\n",
			         &str, str.str_len, str.str_cap);
		string_append_cstring(&str, autism);
		efprintf(stderr, "%p has length %llu, capacity %llu\n",
			         &str, str.str_len, str.str_cap);
		sv_all = string_as_view(&str);

		hash1 = fnv1a((const uint8_t *)str.str_str, str.str_len);
		efprintf(stderr, "\"%.*s\" hashes to %llx\n", str.str_len, str.str_str, hash1);

		hash2 = fnv1a((const uint8_t *)sv->sv_str, sv->sv_len);
		efprintf(stderr, "\"%.*s\" hashes to %llx\n", sv->sv_len, sv->sv_str, hash2);

		hash3 = fnv1a((const uint8_t *)sv_all->sv_str, sv_all->sv_len);
		efprintf(stderr, "\"%.*s\" hashes to %llx\n", sv_all->sv_len, sv_all->sv_str, hash3);
		string_finish(&str);
	}

	{
		const char *data = "I'd like to interject for a moment";
		uint32_t hash = fnv1a((const uint8_t *)data, strlen(data) + 0);
		efprintf(stderr, "\"%s\" hashes to %llx\n", data, hash);
	}

	return 0;
}
