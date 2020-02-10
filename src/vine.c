#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printf.h"
#include "str.h"
#include "memory.h"
#include "alloc.h"
#include "alloc_buf.h"
#include "hash.h"

int
main(void)
{
	const char *data = "I'd like to interject for a moment";
	uint32_t hash = fnv1a((const uint8_t *)data, strlen(data) + 0);
	efprintf(stderr, "\"%s\" hashes to %llx\n", data, hash);
	return 0;
}
