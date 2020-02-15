#include "fibre.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "printf.h"
#include "log.h"
#include "memory.h"

#define MAX_FIBRES 16
#define STACK_SIZE (2048 * 1024)

struct fibre_ctx {
	uint64_t rsp;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbx;
	uint64_t rbp;
};
enum fibre_state {
	TS_UNUSED,
	TS_RUNNING,
	TS_READY
};
struct fibre {
	struct fibre_ctx t_ctx;
	int             t_state;
	char           *t_stack;
};

static struct fibre
fibre_table[MAX_FIBRES];

static struct fibre *
current_fibre;

void
fibre_init(void)
{
	log_info("fibre", "Initialising fibre system\n");
	current_fibre = &fibre_table[0];
	current_fibre->t_state = TS_RUNNING;
	current_fibre->t_stack = NULL;
}

void
fibre_finish(void)
{
	int i;

	log_info("fibre", "Deinitialising fibre system\n");

	for (i = 0; i < MAX_FIBRES; i++) {
		if (fibre_table[i].t_stack != NULL) {
			deallocate(fibre_table[i].t_stack, STACK_SIZE);
		}
	}
}

static int
current_fibre_id(void)
{
	int i;
	struct fibre *fibre;

	for (i = 0, fibre = &fibre_table[0];; i++, fibre++) {
		if (fibre == current_fibre) {
			break;
		}
	}

	return i;
}

int
fibre_current_fibre_id(void)
{
	return current_fibre_id();
}

void
fibre_return(int ret)
{
	log_debug("fibre", "Returning from fibre %d with value %d\n",
	                  current_fibre_id(), ret);

	if (current_fibre != &fibre_table[0]) {
		current_fibre->t_state = TS_UNUSED;
		fibre_yield();
		/* unreachable */
	}
	while (fibre_yield())
		;
	fibre_finish();
	exit(ret);
}

int
fibre_yield(void)
{
	struct fibre *fibre;
	struct fibre_ctx *old, *new;
	int old_id = current_fibre_id();

	fibre = current_fibre;
	while (fibre->t_state != TS_READY) {
		if (++fibre == &fibre_table[MAX_FIBRES]) {
			fibre = &fibre_table[0];
		}
		if (fibre == current_fibre) {
			log_debug("fibre", "Yielding from fibre %d, finishing\n", old_id);
			return 0;
		}
	}

	if (current_fibre->t_state != TS_UNUSED) {
		current_fibre->t_state = TS_READY;
	}
	fibre->t_state = TS_RUNNING;
	old = &current_fibre->t_ctx;
	new = &fibre->t_ctx;
	current_fibre = fibre;
	log_debug("fibre", "Yielding from fibre %d, to fibre %d.\n", old_id, current_fibre_id());
	fibre_switch(old, new);
	return 1;
}

#define FMTREG "0x%010llx"

static void
fibre_stop(void)
{
	fibre_return(0);
}

int
fibre_go(void (*f)(void))
{
	char *stack;
	struct fibre *fibre;
	int i;

	for (i = 0, fibre = &fibre_table[0];; i++, fibre++) {
		if (fibre == &fibre_table[MAX_FIBRES]) {
			log_warning("fibre", "Could not find an empty fibre slot.\n");
			return -1;
		} else if (fibre->t_state == TS_UNUSED) {
			log_debug("fibre", "Found an empty fibre slot, at %d.\n", i);
			break;
		}
	}

	stack = try_allocate(STACK_SIZE);
	if (!stack) {
		return -1;
	}

	*(uint64_t *)&stack[STACK_SIZE -  8] = (uint64_t)fibre_stop;
	*(uint64_t *)&stack[STACK_SIZE - 16] = (uint64_t)f;
	fibre->t_ctx.rsp = (uint64_t)&stack[STACK_SIZE - 16];
	fibre->t_state = TS_READY;
	fibre->t_stack = stack;

	return 0;
}
