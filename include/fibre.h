#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including fibre.h"
#endif
#ifdef VINE_FIBRE_H_INCLUDED
#error "May not include fibre.h more than once"
#endif
#define VINE_FIBRE_H_INCLUDED
extern void fibre_init(struct alloc *alloc, size_t stack_size);
extern void fibre_finish(void);
extern void fibre_return(void);
extern int fibre_yield(void);
/* extern void fibre_go(void (*)(void *), void *); */
extern void fibre_go(void (*)(void));
