#ifdef VINE_PRINTF_H_INCLUDED
#error "May not include printf.h more than once"
#endif
#define VINE_PRINTF_H_INCLUDED
#include "_printf.h"
#ifdef __GNUC__
#pragma GCC poison vfprintf
#pragma GCC poison fprintf
#pragma GCC poison printf
#endif
