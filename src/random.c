#include <stdint.h>
#include <stddef.h>
#include "types.h"
#include "random.h"
#include "hash.h"

void
pcgrng_init(struct pcgrng *rng, u64 initstate, u64 initstream)
{
	rng->state = 0UL;
	rng->stream = (initstream << 1UL) | 1UL;
	(void)pcgrng_rand(rng);
	rng->state += initstate;
	(void)pcgrng_rand(rng);
}

u32
pcgrng_rand(struct pcgrng *rng)
{
	u32 xorshifted, rot;
	u64 oldstate = rng->state;
	rng->state = oldstate * 6364136223846793005UL + rng->stream;
	xorshifted = (u32)(((oldstate >> 18UL) ^ oldstate) >> 27UL);
	rot = (u32)(oldstate >> 59UL);
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u32
pcgrng_randint(struct pcgrng *rng, u32 bound)
{
	u32 threshold = -bound % bound;
	while (1) {
		u32 r = pcgrng_rand(rng);
		if (r >= threshold)
			return r % bound;
	}
}

/* By no means do I claim that this is a good design for a random number
 * generator. However, I am writing it without an internet connection and it
 * will work for the time being. */
static
void
rng_advance(struct rng *rng)
{
	u64 input = (((u64)rng->state) << 32) | ((u64)++rng->counter);
	u64 hash = fnv1a((u8 *)&input, sizeof input);
	rng->state = (u32)(hash >> 32);
	rng->output = (u32)hash;
}

void
rng_init_seed(struct rng *rng, u32 seed)
{
	rng->state = seed;
	rng->counter = 0;
	rng->output = 0;
}

u32
rand32(struct rng *rng)
{
	rng_advance(rng);
	return rng->output;
}

u32
randint(struct rng *rng, u32 max)
{
	return rand32(rng) % max;
}

float
randfloat(struct rng *rng)
{
	(void)rng;
	return 4.0f;
}
