#ifdef VINE_HASH_H_INCLUDED
#error "May not include hash.h more than once"
#endif
#define VINE_HASH_H_INCLUDED
extern uint32_t fnv1a(const uint8_t *data, size_t len);
