#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "printf.h"
#include "log.h"
#include "memory.h"
#include "alloc.h"
#include "alloc_buf.h"
#include "str.h"
#include "hash.h"
#include "fibre.h"
#include "random.h"
#include "object.h"
#include "table.h"

#define MAX_FIBRES 128
#define STACK_SIZE (4 * 1024 * 1024)

static const char *
autism = "I'd just like to interject for a moment.  What you're referring to"
" as Linux, is in fact, GNU/Linux, or as I've recently taken to calling it, GNU"
" plus Linux. Linux is not an operating system unto itself, but rather another"
" free component of a fully functioning GNU system made useful by the GNU"
" corelibs, shell utilities and vital system components comprising a full OS as"
" defined by POSIX.\n"
" Many computer users run a modified version of the GNU system every day,"
" without realizing it.  Through a peculiar turn of events, the version of GNU"
" which is widely used today is often called \"Linux\", and many of its users"
" are not aware that it is basically the GNU system, developed by the"
" GNU Project.\n"
" There really is a Linux, and these people are using it, but it is just a"
" part of the system they use.  Linux is the kernel: the program in the system"
" that allocates the machine's resources to the other programs that you run."
" The kernel is an essential part of an operating system, but useless by"
" itself; it can only function in the context of a complete operating system."
" Linux is normally used in combination with the GNU operating system: the"
" whole system is basically GNU with Linux added, or GNU/Linux.  All the"
" so-called \"Linux\" distributions are really distributions of GNU/Linux.";

static void
test_hash_string(const char *str, size_t len)
{
	u32 hash = fnv1a32((const u8 *)str, len);
	size_t actual = len;
	if (len <= 64) {
		eprintf("\"%.*s\" (len %lu) hashes to %x\n", (int)len, str, actual, hash);
	} else {
		eprintf("\"%.*s...\" (len %lu) hashes to %x\n", 61, str, actual, hash);
	}
}

static int
counter;

#define NDO_PRINT

static void
test_fibre(void)
{
	int i = ++counter;
#ifdef DO_PRINT
	eprintf("Hello, %d!\n", i);
#endif
	if (i < 30) {
		fibre_go(test_fibre);
		fibre_yield();
	}
	fibre_yield();
#ifdef DO_PRINT
	eprintf("Goodbye, %d!\n", i);
#endif
	fibre_return(0);
}

int
main(void)
{
	counter = 0;


	log_set_loglevel(LOG_INFO);

	{
		struct rng rng;

		rng_init_seed(&rng, (u32)(u64)(void *)&rng);

		eprintf("fnv1a:\n");
		eprintf("0x%08x\n", rand32(&rng));
		eprintf("0x%08x\n", rand32(&rng));
		eprintf("0x%08x\n", rand32(&rng));
		eprintf("0x%08x\n", rand32(&rng));
		eprintf("0x%08x\n", rand32(&rng));
	}

	{
		struct pcgrng rng;

		pcgrng_init(&rng, (u64)(void *)&rng, 0);

		eprintf("pcg:\n");
		eprintf("0x%08x\n", pcgrng_rand(&rng));
		eprintf("0x%08x\n", pcgrng_rand(&rng));
		eprintf("0x%08x\n", pcgrng_rand(&rng));
		eprintf("0x%08x\n", pcgrng_rand(&rng));
		eprintf("0x%08x\n", pcgrng_rand(&rng));
	}

	{
		struct object_vtable **vobj1 = vobject_from_int(1);
		struct object_vtable **vobj2 = vobject_from_cstr("2");
		eprintf("1 == 1: %d\n", vobject_equal(vobj1, vobj1));
		eprintf("1 == '2': %d\n", vobject_equal(vobj1, vobj2));
		eprintf("'2' == '2': %d\n", vobject_equal(vobj2, vobj2));
		vobject_destroy(vobj1, &sys_alloc);
		vobject_destroy(vobj2, &sys_alloc);
		/*
		deallocate(vobj1, sizeof(struct int_object));
		deallocate(vobj2, sizeof(struct cstr_object));
		*/
	}

	{
		union object obj1, obj2;
		object_init_from_int(&obj1, 1);
		object_init_from_cstr(&obj2, "2");
		eprintf("1 == 1: %d\n", object_equal(obj1, obj1));
		eprintf("1 == '2': %d\n", object_equal(obj1, obj2));
		eprintf("'2' == '2': %d\n", object_equal(obj2, obj2));
	}

	{
		union object t;
		object_init_as_table(&t, &sys_alloc, 16);
		eprintf("table: %s\n", object_typename(t));
		object_destroy(t, &sys_alloc);
	}

	{
		struct string str;
		struct strview *sv, *sv_all;
		string_init_with(&str, &sys_alloc, 16);
		string_append_cstring(&str, autism);
		string_expand_to(&str, 4096);
		sv = string_as_view(&str);
		string_append_cstring(&str, autism);
		string_append_cstring(&str, autism);
		sv_all = string_as_view(&str);

		test_hash_string(str.str_str, str.str_len);
		string_finish(&str);

		test_hash_string(sv->sv_str, sv->sv_len);
		test_hash_string(sv_all->sv_str, sv_all->sv_len);
		strview_destroy(sv);
		strview_destroy(sv_all);
	}

	{
		const char *data = "I'd just like to interject for a moment";
		test_hash_string(data, strlen(data));
		fibre_init(&mmap_alloc, STACK_SIZE);
		fibre_go(test_fibre);
		fibre_return(0);
		/* test_hash_string(data, strlen(data) - 5); */
		/* fibre_finish(); */
	}

	return 0;
}
