#ifdef VINE_CHECKED_H_INCLUDED
#error "May not include checked.h more than once"
#endif
#define VINE_CHECKED_H_INCLUDED
int try_add_sz(size_t, size_t, size_t *);
size_t add_sz(size_t, size_t);
int try_mul_sz(size_t, size_t, size_t *);
size_t mul_sz(size_t, size_t);
