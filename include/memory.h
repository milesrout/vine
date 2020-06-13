#ifdef VINE_MEMORY_H_INCLUDED
#error "May not include memory.h more than once"
#endif
#define VINE_MEMORY_H_INCLUDED
#include "_memory.h"
#ifdef __GNUC__
#pragma GCC poison malloc
#pragma GCC poison calloc
#pragma GCC poison realloc
#pragma GCC poison emalloc
#pragma GCC poison ecalloc
#pragma GCC poison erealloc
#pragma GCC poison free
#endif
