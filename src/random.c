#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "random.h"
#include "hash.h"

void
rng_init(struct rng *rng, u64 initstate, u64 initstream)
{
	rng->rng_state = 0UL;
	rng->rng_stream = (initstream << 1UL) | 1UL;
	(void)rng_rand(rng);
	rng->rng_state += initstate;
	(void)rng_rand(rng);
}

u32
rng_rand(struct rng *rng)
{
	u32 xorshifted, rot;
	u64 oldstate = rng->rng_state;
	rng->rng_state = oldstate * 6364136223846793005UL + rng->rng_stream;
	xorshifted = (u32)(((oldstate >> 18UL) ^ oldstate) >> 27UL);
	rot = (u32)(oldstate >> 59UL);
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u32
rng_randint(struct rng *rng, u32 bound)
{
	u32 threshold = -bound % bound;
	while (1) {
		u32 r = rng_rand(rng);
		if (r >= threshold)
			return r % bound;
	}
}

float
rng_randfloat(struct rng *rng)
{
	(void)rng;
	return 4.0f;
}
