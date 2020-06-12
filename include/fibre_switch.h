#ifdef VINE_FIBRE_SWITCH_H_INCLUDED
#error "May not include fibre_switch.h more than once"
#endif
#define VINE_FIBRE_SWITCH_H_INCLUDED
struct fibre_ctx;
extern void fibre_switch(struct fibre_ctx *old, struct fibre_ctx *new);
