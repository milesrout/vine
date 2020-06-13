#ifdef VINE_UTIL_H_INCLUDED
#error "May not include util.h more than once"
#endif
#define VINE_UTIL_H_INCLUDED
#define container_of(type, member, ptr) (type *)(void *)(((char *)(ptr)) - offsetof(type, member))
