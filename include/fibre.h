#ifdef VINE_FIBRE_H_INCLUDED
#error "May not include fibre.h more than once"
#endif
#define VINE_FIBRE_H_INCLUDED
struct fibre_ctx;
struct fibre;
extern void fibre_init(void);
extern void fibre_finish(void);
extern void fibre_return(int);
extern void fibre_switch(struct fibre_ctx *old, struct fibre_ctx *new);
extern int fibre_yield(void);
extern int fibre_go(void (*)(void));
extern int fibre_current_fibre_id(void);
