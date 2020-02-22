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
 * values for all callee-saved registers, and the stack pointer. Because we
 * switch fibres using a function call, the compiler takes care of saving
 * everything else.
 *
 * This struct will need to be extended with additional items over time. For
 * example, if/when fibre-local storage is implemented, there will need to be
 * pointer in this structure to that storage.
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
	/* a fibre that is waiting for an I/O operation to be completed */
	FS_WAITING,
	/* a fibre that represents a valid, suspended execution context */
	FS_READY
};
struct fibre {
	struct fibre_ctx ctx;
	int state;
	char *stack;
};

/*
 * This value is calculated so that each fibre_store_node should be page-sized.
 * The allocation for a node includes the array of struct fibres, so:
 *     sizeof(fibre_store_node)
 *         = 2 * sizeof(void *) + FIBRES_PER_NODE * sizeof(struct fibre)
 *
 * Currently, sizeof(struct fibre) is 72 bytes, so this value should be 56.
 * The array takes up 4032 bytes, leaving 64 bytes for the two pointers.
 */
#define FIBRES_PER_NODE 56
#define FIBRE_STORE_NODE_BLOCK (sizeof(struct fibre_store_node) + sizeof(struct fibre) * FIBRES_PER_NODE)

struct fibre_store_node {
	struct fibre *fibres;
	struct fibre_store_node *next;
};

struct fibre_store {
	size_t stack_size;
	struct alloc *alloc;
	struct fibre_store_node *store;
};

static
struct fibre_store_node *
fibre_store_node_create(struct alloc *alloc)
{
	size_t i;
	struct fibre_store_node *node;

	void *p = allocate_with(alloc, FIBRE_STORE_NODE_BLOCK);

	node = (struct fibre_store_node *)p;
	node->fibres = (void *)((char *)p + sizeof(struct fibre_store_node));

	for (i = 0; i < FIBRES_PER_NODE; i++) {
		node->fibres[i].state = FS_EMPTY;
		node->fibres[i].stack = NULL;
	}

	node->next = NULL;

	return node;
}

static
void
fibre_store_node_destroy(struct fibre_store *store, struct fibre_store_node *node)
{
	if (node->next != NULL) {
		fibre_store_node_destroy(store, node->next);
	}
	deallocate_with(store->alloc, node, FIBRE_STORE_NODE_BLOCK);
}

static
struct fibre *
fibre_store_get_first_empty(struct fibre_store *store)
{
	struct fibre_store_node *node = store->store;
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
	node->next = fibre_store_node_create(store->alloc);

	return &node->next->fibres[0];
}

static
struct fibre *
fibre_store_get_next_ready(struct fibre_store *store, size_t i)
{
	struct fibre_store_node *node = store->store;
	size_t k = i;

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

	node = store->store;

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

static struct fibre *current_fibre;
static struct fibre *main_fibre;
static struct fibre_store global_fibre_store;

void
fibre_init(struct alloc *alloc, size_t stack_size)
{
	log_info("fibre", "Initialising fibre system with 0x%llxB-sized stacks\n",
	                  stack_size / 1024);
	if (FIBRE_STORE_NODE_BLOCK > 4096) {
		log_warning("fibre", "fibre_store_node is too big to fit in a page: lower FIBRES_PER_NODE\n");
	} else if (FIBRE_STORE_NODE_BLOCK + sizeof(struct fibre) <= 4096) {
		log_notice("fibre", "fibre_store_node could be bigger: raise FIBRES_PER_NODE\n");
	} else {
		log_info("fibre", "fibre_store_node fits perfectly in a page\n");
	}

	global_fibre_store.stack_size = stack_size;
	global_fibre_store.alloc = alloc;
	global_fibre_store.store = fibre_store_node_create(alloc);
	current_fibre = main_fibre = global_fibre_store.store->fibres;
	main_fibre->state = FS_ACTIVE;
	main_fibre->stack = NULL;
}


void
fibre_finish(void)
{
	struct fibre_store_node *node = global_fibre_store.store;
	size_t i;

	log_info("fibre", "Deinitialising fibre system\n");

	while (node != NULL) {
		for (i = 0; i < FIBRES_PER_NODE; i++) {
			if (node->fibres[i].stack != NULL) {
				deallocate_with(global_fibre_store.alloc,
					node->fibres[i].stack,
					global_fibre_store.stack_size);
			}
		}
		node = node->next;
	}

	fibre_store_node_destroy(&global_fibre_store, global_fibre_store.store);
}

static
size_t
current_fibre_id(void)
{
	size_t i = 0, k;
	struct fibre_store_node *node = global_fibre_store.store;

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
		/* TODO: When we finish with a fibre, we should mark its stack
		 * as no longer needed. We could deallocate it, but constantly
		 * allocating and deallocating stacks is likely to be wasteful.
		 * However, reusing stacks has the potential for information
		 * leakage. MADV_DONTNEED should zero-fill the pages if they're
		 * ever reused, however. We need an allocator-dependent
		 * equivalent of:
		 *
		 * madvise(current_fibre->stack, MADV_DONTNEED);
		 *
		 * Or we need to hardcode the use of the mmap allocator for
		 * allocating stacks.
		 */
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
	struct fibre_ctx *old, *new;
	size_t old_id = current_fibre_id();
	struct fibre *fibre = fibre_store_get_next_ready(&global_fibre_store, old_id);

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
	size_t size = global_fibre_store.stack_size;
	struct fibre *fibre = fibre_store_get_first_empty(&global_fibre_store);

	if (fibre->stack == NULL) {
		stack = allocate_with(global_fibre_store.alloc, size);
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
