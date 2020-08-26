#ifdef VINE_CHECKED_H_INCLUDED
#error "May not include checked.h more than once"
#endif
#define VINE_CHECKED_H_INCLUDED
int try_add_sz(size_t, size_t, size_t *);
size_t add_sz(size_t, size_t);
int try_mul_sz(size_t, size_t, size_t *);
size_t mul_sz(size_t, size_t);
int try_add_uip(uintptr_t x, size_t, size_t *);
uintptr_t add_uip(uintptr_t, size_t);
size_t align_sz(size_t, size_t);
char *align_ptr(char *, size_t);
