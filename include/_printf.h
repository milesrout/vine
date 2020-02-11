#ifdef VINE__PRINTF_H_INCLUDED
#error "May not include _printf.h more than once"
#endif
#define VINE__PRINTF_H_INCLUDED
void evfprintf(FILE *file, const char *fmt, va_list args);
void efprintf(FILE *file, const char *fmt, ...);
void eprintf(const char *fmt, ...);
