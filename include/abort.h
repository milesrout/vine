/* this is a little bit of a tricky situation. abort.h may be included with or
 * without printf.h being included first. If printf.h has been included first,
 * then we can't use 'printf' in the '__attribute__((format(printf' nonsense as
 * it is poisoned. However we cannot rely on printf.h being included first to
 * define the attribute_format_printf(a, b) macro before the poison. So we
 * define the macro if printf.h hasn't been included yet and undefine it at the
 * end of the header.
 */
#ifndef VINE_PRINTF_H_INCLUDED
#ifdef __GNUC__
#define attribute_format_printf(a, b) __attribute__((format(printf, a, b)))
#else
#define attribute_format_printf(a, b)
#endif
#endif
#ifdef VINE_ABORT_H_INCLUDED
#error "May not include abort.h more than once"
#endif
#define VINE_ABORT_H_INCLUDED
#ifdef __GNUC__
#define attribute_noreturn __attribute__((noreturn))
#else
#define attribute_noreturn
#endif
attribute_format_printf(1, 2) extern void abort_with_error(const char *fmt, ...) attribute_noreturn;
extern void breakpoint(void);
#if !defined(NDEBUG) || !defined(__GNUC__)
#define assert1(condition)          assert2(condition, #condition)
#define assert2(condition, message) do { if (!(condition)) abort_with_error("%s:%d: %s\n", __FILE__, __LINE__, message); } while (0)
#else
#define assert1(condition)          do { if (!(condition)) __builtin_unreachable(); } while (0)
#define assert2(condition, message) assert1(condition)
#endif
#ifndef VINE_PRINTF_H_INCLUDED
#undef attribute_format_printf
#endif
