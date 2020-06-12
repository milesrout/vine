#ifdef VINE_ABORT_H_INCLUDED
#error "May not include abort.h more than once"
#endif
#define VINE_ABORT_H_INCLUDED
extern void abort_with_error(const char *fmt, ...);
#define assert1(condition) do { if (!(condition)) abort_with_error("%s:%d: %s\n", __FILE__, __LINE__, #condition); } while (0)
#define assert2(condition, message) do { if (!(condition)) abort_with_error("%s:%d: %s\n", __FILE__, __LINE__, message); } while (0)
