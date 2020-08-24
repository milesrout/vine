#ifdef VINE_TYPES_H_INCLUDED
#error "May not include types.h more than once"
#endif
#define VINE_TYPES_H_INCLUDED
typedef int8_t  z8;
typedef int16_t z16;
typedef int32_t z32;
typedef int64_t z64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
typedef __int128 z128;
typedef unsigned __int128 u128;
#pragma GCC diagnostic pop
#define Z128_LITERAL(a,b) ((((z128)(a)) << 64) | ((z128)(b)))
#define U128_LITERAL(a,b) ((((u128)(a)) << 64) | ((u128)(b)))
#endif
#ifdef __GNUC__
#pragma GCC poison int8_t
#pragma GCC poison int16_t
#pragma GCC poison int32_t
#pragma GCC poison int64_t
#pragma GCC poison uint8_t
#pragma GCC poison uint16_t
#pragma GCC poison uint32_t
#pragma GCC poison uint64_t
#endif
