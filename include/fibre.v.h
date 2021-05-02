//require alloc.h
//provide fibre.h
extern void fibre_init(struct alloc *alloc, size_t stack_size);
extern void fibre_finish(void);
extern void fibre_return(void);
extern int fibre_yield(void);
/* extern void fibre_go(void (*)(void *), void *); */
extern void fibre_go(void (*)(void));
