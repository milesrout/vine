#ifdef VINE_RANDOM_H_INCLUDED
#error "May not include random.h more than once"
#endif
#define VINE_RANDOM_H_INCLUDED
struct rng {
};
extern void rng_init(struct rng *rng);
extern uint32_t random(struct rng *rng);
extern uint32_t randint(struct rng *rng, uint32_t max);
extern float randfloat(struct rng *rng);
struct rng64 {
};
extern void rng64_init(struct rng64 *rng);
extern uint64_t random64(struct rng64 *rng);
extern uint64_t randint64(struct rng64 *rng, uint64_t max);
extern double randfloat64(struct rng64 *rng);
