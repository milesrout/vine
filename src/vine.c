#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "types.h"
#include "abort.h"
#include "eprintf.h"
#include "log.h"
#include "memory.h"
#include "alloc.h"
#include "alloc_buf.h"
#include "slab_pool.h"
#include "str.h"
#include "hash.h"
#include "fibre.h"
#include "random.h"
#include "object.h"
#include "table.h"
#include "heapstring.h"

#define STACK_SIZE (4 * 1024 * 1024)

static
const char *
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

static
void
test_hash_string(const char *str, size_t len)
{
	u64 hash = fnv1a((const u8 *)str, len);
	size_t actual = len;
	if (len <= 64) {
		eprintf("\"%.*s\" (len %lu) hashes to %lx\n", (int)len, str, actual, hash);
	} else {
		eprintf("\"%.*s...\" (len %lu) hashes to %lx\n", 61, str, actual, hash);
	}
}

static
int
counter;

#define NDO_PRINT

static
void
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
	fibre_return();
}

static void test_slab(void);

int
main(void)
{
	size_t page_size;
	counter = 0;

	log_init();
	log_set_loglevel(LOG_INFO);
	log_set_system_loglevel("alloc_buf", LOG_DEBUG);

	page_size = (size_t)sysconf(_SC_PAGESIZE);
	if (page_size != PAGE_SIZE) {
		abort_with_error("Page size must be %lu, but is %lu\n",
			PAGE_SIZE, page_size);
	}

	test_slab();

	{
		struct rng rng;

		rng_init(&rng, (u64)(void *)&rng, 0);

		eprintf("pcg:\n");
		eprintf("0x%08x\n", rng_rand(&rng));
		eprintf("0x%08x\n", rng_rand(&rng));
		eprintf("0x%08x\n", rng_rand(&rng));
		eprintf("0x%08x\n", rng_rand(&rng));
		eprintf("0x%08x\n", rng_rand(&rng));
	}

	{
		struct object_vtable **vobj1 = vobject_from_int(1);
		struct object_vtable **vobj2 = vobject_from_cstr("2");
		eprintf("1 == 1: %d\n", vobject_equal(vobj1, vobj1));
		eprintf("1 == '2': %d\n", vobject_equal(vobj1, vobj2));
		eprintf("'2' == '2': %d\n", vobject_equal(vobj2, vobj2));
		vobject_destroy(vobj1, &sys_alloc);
		vobject_destroy(vobj2, &sys_alloc);
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
		struct heapstring *hs1 = heapstring_create(HEAPSTRING_PAGE_CAP,
			&mmap_alloc);
		heapstring_destroy(hs1, &mmap_alloc);
	}

	{
		char buf[4096];
		struct buf_alloc buf_alloc;
		struct string str;
		struct strview *sv, *sv_all;
		buf_alloc_init(&buf_alloc, buf, 4096);
		string_init_with(&str, &buf_alloc.ba_alloc, 16);
		str.str_viewalloc = &sys_alloc;
		string_append_cstring(&str, autism);
		string_expand_to(&str, 4096);
		sv = string_as_view(&str);
		string_append_cstring(&str, autism);
		string_append_cstring(&str, autism);
		sv_all = string_as_view(&str);

		test_hash_string(str.str_str, str.str_len);
		eprintf("refcnt before finish: %lu\n", str.str_refcnt);
		string_finish(&str);
		eprintf("refcnt after finish: %lu\n", str.str_refcnt);

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
		fibre_return();
		test_hash_string(data, strlen(data) - 5);
		/* fibre_finish(); */
	}

	log_finish();
	return 0;
}

#define FOO_ALIGN __alignof__(struct foo)
#define FOO_SIZE sizeof(struct foo)

struct foo {
	struct string str;
	char x;
	size_t y;
	char z;
};

static
void
foo_init(void *foo)
{
	eprintf("foo_init %p\n", foo);
	string_init((struct string *)foo);
	eprintf("foo_init %d\n", *(int*)foo);
}

static
void
foo_finish(void *foo)
{
	eprintf("foo_finish %p\n", foo);
	string_finish((struct string *)foo);
}

static
void
test_slab(void)
{
	struct slab_pool sa;
	struct foo *f[3];

	slab_pool_init(&sa, FOO_ALIGN, FOO_SIZE, &foo_init, &foo_finish);

	eprintf("FOO_ALIGN=%lu\n", FOO_ALIGN);
	eprintf("FOO_SIZE=%lu\n", FOO_SIZE);

	f[0] = slab_object_create(&sa);
	f[1] = slab_object_create(&sa);
	f[2] = slab_object_create(&sa);
	slab_object_destroy(&sa, f[0]);
	slab_object_destroy(&sa, f[1]);
	slab_object_destroy(&sa, f[2]);

	slab_pool_finish(&sa);
}
