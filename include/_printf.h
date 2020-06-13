#ifdef VINE__PRINTF_H_INCLUDED
#error "May not include _printf.h more than once"
#endif
#define VINE__PRINTF_H_INCLUDED
#ifdef __GNUC__
#define attribute_format_printf(a, b) __attribute__((format(printf, a, b)))
#else
#define attribute_format_printf(a, b)
#endif
attribute_format_printf(2, 0) extern void evfprintf(FILE *file, const char *fmt, va_list args);
attribute_format_printf(2, 3) extern void efprintf(FILE *file, const char *fmt, ...);
attribute_format_printf(1, 2) extern void eprintf(const char *fmt, ...);
