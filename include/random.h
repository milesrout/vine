#ifdef VINE_RANDOM_H_INCLUDED
#error "May not include random.h more than once"
#endif
#define VINE_RANDOM_H_INCLUDED
struct pcgrng {
	u64 state;
	u64 stream;
};
extern void pcgrng_init(struct pcgrng *, u64, u64);
extern u32  pcgrng_rand(struct pcgrng *);
extern u32  pcgrng_randint(struct pcgrng *, u32);
struct rng {
	u32 state;
	u32 counter;
	u32 output;
};
extern void rng_init_seed(struct rng *rng, u32 seed);
extern u32 rand32(struct rng *rng);
extern u32 randint(struct rng *rng, u32 max);
extern float randfloat(struct rng *rng);
