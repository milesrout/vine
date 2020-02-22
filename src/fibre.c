#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "alloc.h"
#include "fibre.h"
#include "printf.h"
#include "abort.h"
#include "log.h"
#include "memory.h"

/*
 * This structure represents an execution context, namely it contains the
 * values for all callee-saved registers. Because we switch fibres using a
 * function call, the compiler takes care of saving everything else.
 *
 * This struct will need to be extended with additional items over time. For
 * example, if/when fibre-local storage is implemented, there will need to be
 * an FLS pointer stored in this structure.
 */
struct fibre_ctx {
	uint64_t rsp;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbx;
	uint64_t rbp;
};
/*
 * A function defined in fibre.S that actually performs the context switch.
 */
extern void fibre_switch(struct fibre_ctx *old, struct fibre_ctx *new);

enum fibre_state {
	/* a fibre that represents a possibly-uninitialised execution context */
	FS_EMPTY,
	/* the fibre that represents the current execution context */
	FS_ACTIVE,
	/* a fibre that represents a valid, suspended execution context */
	FS_READY
};
struct fibre {
	struct fibre_ctx ctx;
	int state;
	char *stack;
};

/*
 * This value is calculated so that each fibre_queue_node should be page-sized.
 * The allocation for a node includes the array of struct fibres, so:
 *     sizeof(fibre_queue_node)
 *         = 2 * sizeof(void *) + FIBRES_PER_NODE * sizeof(struct fibre)
 *
 * Currently, sizeof(struct fibre) is 72 bytes, so this value should be 56.
 * The array takes up 4032 bytes, leaving 64 bytes for the two pointers.
 */
#define FIBRES_PER_NODE 56
#define FIBRE_QUEUE_NODE_BLOCK (sizeof(struct fibre_queue_node) + sizeof(struct fibre) * FIBRES_PER_NODE)

struct fibre_queue_node {
	struct fibre *fibres;
	struct fibre_queue_node *next;
};
struct fibre_queue {
	size_t stack_size;
	struct alloc *alloc;
	struct fibre_queue_node *list;
};

/*
 * This function is used in the initialisation of fibre_queue so it should only
 * use the alloc and stack_size members.
 */
static
struct fibre_queue_node *
fibre_queue_node_create(struct fibre_queue *list)
{
	size_t i;
	struct fibre_queue_node *node;

	void *p = allocate_with(list->alloc, FIBRE_QUEUE_NODE_BLOCK);

	node = (struct fibre_queue_node *)p;
	node->fibres = (void *)((char *)p + sizeof(struct fibre_queue_node));

	for (i = 0; i < FIBRES_PER_NODE; i++) {
		node->fibres[i].state = FS_EMPTY;
		node->fibres[i].stack = NULL;
	}

	node->next = NULL;

	return node;
}

static struct fibre *current_fibre;
static struct fibre *main_fibre;
static struct fibre_queue global_fibre_queue;

static
struct fibre *
fibre_queue_get_first_empty(void)
{
	struct fibre_queue_node *node = global_fibre_queue.list;
	size_t i;

	/* This loop is structured the way it is so that we can do something to
	 * the last node after the loop finished (unless we early return).
	 */
	while (1) {
		for (i = 0; i < FIBRES_PER_NODE; i++) {
			if (node->fibres[i].state == FS_EMPTY) {
				return node->fibres + i;
			}
		}
		if (node->next == NULL) {
			goto break_both;
		}
		node = node->next;
	}
break_both:

	/* We didn't find one, so we need to allocate a new node. */
	node->next = fibre_queue_node_create(&global_fibre_queue);

	return &node->next->fibres[0];
}

void
fibre_init(struct alloc *alloc, size_t stack_size)
{
	log_info("fibre", "Initialising fibre system with %llu-sized stacks\n",
	                  stack_size);
	if (FIBRE_QUEUE_NODE_BLOCK > 4096) {
		log_warning("fibre", "fibre_queue_node is too big: lower FIBRES_PER_NODE\n");
	} else if (FIBRE_QUEUE_NODE_BLOCK + sizeof(struct fibre) <= 4096) {
		log_notice("fibre", "fibre_queue_node could be bigger: raise FIBRES_PER_NODE\n");
	} else {
		log_info("fibre", "fibre_queue_node fits perfectly in a page\n");
	}

	global_fibre_queue.stack_size = stack_size;
	global_fibre_queue.alloc = alloc;
	global_fibre_queue.list = fibre_queue_node_create(&global_fibre_queue);
	current_fibre = main_fibre = global_fibre_queue.list->fibres;
	main_fibre->state = FS_ACTIVE;
	main_fibre->stack = NULL;
}

static
void
fibre_queue_node_destroy(struct fibre_queue_node *node)
{
	if (node->next != NULL) {
		fibre_queue_node_destroy(node->next);
	}
	deallocate_with(global_fibre_queue.alloc, node, FIBRE_QUEUE_NODE_BLOCK);
}

void
fibre_finish(void)
{
	struct fibre_queue_node *node = global_fibre_queue.list;
	size_t i;

	log_info("fibre", "Deinitialising fibre system\n");

	while (node != NULL) {
		for (i = 0; i < FIBRES_PER_NODE; i++) {
			if (node->fibres[i].stack != NULL) {
				deallocate_with(global_fibre_queue.alloc,
					node->fibres[i].stack,
					global_fibre_queue.stack_size);
			}
		}
		node = node->next;
	}

	fibre_queue_node_destroy(global_fibre_queue.list);
}

static
size_t
current_fibre_id(void)
{
	size_t i = 0, k;
	struct fibre_queue_node *node = global_fibre_queue.list;

	while (node != NULL) {
		for (k = 0; k < FIBRES_PER_NODE; k++, i++) {
			if (&node->fibres[k] == current_fibre) {
				goto break_both;
			}
		}
		node = node->next;
	}
break_both:

	return i;
}

size_t
fibre_current_fibre_id(void)
{
	return current_fibre_id();
}

void
fibre_return(int ret)
{
	log_debug("fibre", "Returning from fibre %llu with value %d\n",
	                   current_fibre_id(), ret);

	if (current_fibre != main_fibre) {
		current_fibre->state = FS_EMPTY;
		fibre_yield();
		/* unreachable */
	}
	while (fibre_yield())
		;
	fibre_finish();
	exit(ret);
}

static
struct fibre *
fibre_queue_get_next_ready(size_t i)
{
	struct fibre_queue_node *node = global_fibre_queue.list;
	size_t k = i;

	log_debug("fibre", "i=%llu\n", i);

	/* Loop through until we get to the current fibre */
	while (node != NULL) {
		/* Once we get to it */
		if (k < FIBRES_PER_NODE) {

			/* Find a ready fibre after it if one exists */
			for (k++; k < FIBRES_PER_NODE; k++) {
				if (node->fibres[k].state == FS_READY) {
					return &node->fibres[k];
				}
			}

			/* Or loop through the remaining nodes to find one */
			node = node->next;
			while (node != NULL) {
				for (k = 0; k < FIBRES_PER_NODE; k++) {
					if (node->fibres[k].state == FS_READY) {
						return &node->fibres[k];
					}
				}
				node = node->next;
			}

			/* Start over from the beginning and go through to current */
			break;
		}
		k -= FIBRES_PER_NODE;
		node = node->next;
	}

	node = global_fibre_queue.list;

	/* Loop through until we get to the current fibre */
	while (node != NULL) {
		/* Once we get to it */
		if (k < FIBRES_PER_NODE) {
			/* Find a ready fibre before it */
			for (k = 0; k < i; k++) {
				if (node->fibres[k].state == FS_READY) {
					return &node->fibres[k];
				}
			}

			/* Okay, there isn't one. So there are no ready fibres
			 * at all. We just return NULL to indicate we're done.
			 */
			return NULL;
		}

		/* Otherwise, just loop through all the fibres in this node */
		for (k = 0; k < FIBRES_PER_NODE; k++) {
			if (node->fibres[k].state == FS_READY) {
				return &node->fibres[k];
			}
		}
		i -= FIBRES_PER_NODE;
		node = node->next;
	}

	return NULL;
}

int
fibre_yield(void)
{
	struct fibre_ctx *old, *new;
	size_t old_id = current_fibre_id();
	struct fibre *fibre = fibre_queue_get_next_ready(old_id);

	if (fibre == NULL) {
		log_debug("fibre", "Yielding from fibre %llu, finishing\n", old_id);
		return 0;
	}

	if (current_fibre->state != FS_EMPTY) {
		current_fibre->state = FS_READY;
	}
	fibre->state = FS_ACTIVE;
	old = &current_fibre->ctx;
	new = &fibre->ctx;
	current_fibre = fibre;
	log_debug("fibre", "Yielding from fibre %llu to fibre %llu.\n", old_id, current_fibre_id());
	fibre_switch(old, new);
	return 1;
}

#define FMTREG "0x%010llx"

static void
fibre_stop(void)
{
	fibre_return(0);
}

void
/* fibre_go(void (*f)(void *), void *data) */
fibre_go(void (*f)(void))
{
	char *stack;
	size_t size = global_fibre_queue.stack_size;
	struct fibre *fibre = fibre_queue_get_first_empty();

	if (fibre->stack == NULL) {
		stack = allocate_with(global_fibre_queue.alloc, size);
	} else {
		stack = fibre->stack;
	}

	*(uint64_t *)&stack[size -  8] = (uint64_t)fibre_stop;
	*(uint64_t *)&stack[size - 16] = (uint64_t)f;
	fibre->ctx.rsp = (uint64_t)&stack[size - 16];
	/* Obviously this doesn't actually work! OBVIOUSLY! We need to use an
	 * assembly function to put this pointer into rsi before the first call
	 * to this fibre.
	 *     fibre->ctx.rsi = (uint64_t)data;
	 */
	fibre->state = FS_READY;
	fibre->stack = stack;
}
