//provide random.h
struct rng {
	u64 rng_state;
	u64 rng_stream;
};
extern void rng_init(struct rng *, u64, u64);
extern u32  rng_rand(struct rng *);
extern u32  rng_randint(struct rng *, u32);
extern float rng_randfloat(struct rng *);
