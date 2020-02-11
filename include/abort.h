#ifdef VINE_ABORT_H_INCLUDED
#error "May not include abort.h more than once"
#endif
#define VINE_ABORT_H_INCLUDED
extern void abort_with_error(const char *fmt, ...);
