#include <unistd.h>
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

#include "fibre_switch.h"

static struct {
	long int stack_allocs;
	long int fibre_go_calls;
} fibre_stats = {0};

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

enum fibre_prio {
	/* a fibre used for ui or other interactive functionality */
	FP_HIGH,
	/* any other fibre */
	FP_NORMAL,
	/* a fibre used for non-latency-sensitive background tasks */
	FP_BACKGROUND,
	/* the number of priority levels */
	FP_NUM_PRIOS
};

/*
 * This structure represents a fibre. It includes the execution context (struct
 * fibre_ctx) along with the state of the fibre and a pointer to the fibre's
 * stack. This is everything you need to know whether a fibre can be switched
 * to and how to switch to it.
 */
struct fibre {
	struct fibre_ctx ctx;
	int state;
	int prio;
	char *stack;
};

static
void
fibre_setup(struct fibre *fibre)
{
	/* ctx is invalid when state = EMPTY */
	fibre->state = FS_EMPTY;
	/* prio is invalid when state = EMPTY */
	fibre->stack = NULL;
}

/*
 * This is a node in an intrusive bidirectional linked list of struct fibres.
 */
struct fibre_store_node {
	struct fibre_store_node *prev, *next;
	struct fibre fibre;
};

#define container_of(type, member, ptr) (type *)(void *)(((char *)(ptr)) - offsetof(type, member))

/*
 * This is the control block for a linked list of struct fibres.
 * it has two valid states:
 *   start===NULL & end===NULL -> list is empty
 *   start=/=NULL & end=/=NULL -> list is non-empty
 */
struct fibre_store_list {
	struct fibre_store_node *start, *end;
};

static
void
fibre_store_list_init(struct fibre_store_list *list)
{
	list->start = list->end = NULL;
}

static
void
fibre_store_list_enqueue(struct fibre_store_list *list, struct fibre *fibre)
{
	struct fibre_store_node *node = container_of(struct fibre_store_node, fibre, fibre);

	/* require that the fibre is not already in a list */
	assert1(node->next == NULL);
	assert1(node->prev == NULL);

	if (list->start == NULL || list->end == NULL) {
		assert1(list->start == NULL);
		assert1(list->end == NULL);

		list->start = node;
		list->end = node;
	} else {
		assert1(list->start != NULL);
		assert1(list->end != NULL);

		list->start->prev = node;
		node->next = list->start;
		list->start = node;
	}
}

static
struct fibre *
fibre_store_list_dequeue(struct fibre_store_list *list)
{
	struct fibre_store_node *node;

	if (list->start == NULL || list->end == NULL) {
		assert1(list->start == NULL);
		assert1(list->end == NULL);

		return NULL;
	}

	/* require that the list is non-empty */
	assert1(list->start != NULL);
	assert1(list->end != NULL);

	/* these should just generally be true of *every* list */
	assert1(list->start->prev == NULL);
	assert1(list->end->next == NULL);

	node = list->end;

	if (node->prev == NULL) {
		assert1(list->end == list->start);
		list->start = list->end = NULL;
	} else {
		assert1(list->end != list->start);
		node->prev->next = NULL;
		list->end = node->prev;
		node->prev = NULL;
	}

	return &node->fibre;
}

/*
 * This value is calculated so that each fibre_store_block should be page-sized.
 * Currently, sizeof(fibre_store_node) is 88 bytes, so this value should be 46.
 * The block takes up 4048 bytes.
 */
#define FIBRE_STORE_NODES_PER_BLOCK 46

/*
 * struct fibres are allocated in page-sized blocks, which at the moment are
 * never deallocated, but the struct fibres within them can be reused.
 */
struct fibre_store_block {
	struct fibre_store_node nodes[FIBRE_STORE_NODES_PER_BLOCK];
};

/*
 * there is a list of ready fibres for each priority level plus a list of
 * empty fibres.
 */
struct fibre_store {
	size_t stack_size;
	struct alloc *alloc;
	struct fibre_store_list lists[FP_NUM_PRIOS + 1];
};

#define MAX (FIBRE_STORE_NODES_PER_BLOCK - 1)

static
struct fibre_store_block *
fibre_store_block_create(struct alloc *alloc)
{
	struct fibre_store_block *block = allocate_with(alloc, sizeof *block);
	size_t i;

	/* set up the nodes in the block to form a bidirectional linked list */
	for (i = 0; i <= MAX; i++) {
		block->nodes[i].prev = (i == 0) ? NULL : &block->nodes[i - 1];
		block->nodes[i].next = (i == MAX) ? NULL : &block->nodes[i + 1];
		fibre_setup(&block->nodes[i].fibre);
	}

	return block;
}

#undef MAX

/*
 * This takes fibres from the start of the list intead of the end, treating the
 * empties list as a stack. The other lists are treated as queues.
 */
static
struct fibre *
fibre_store_get_first_empty(struct fibre_store *store)
{
	struct fibre_store_list *empties_list = &store->lists[FP_NUM_PRIOS];
	struct fibre_store_node *node;

	if (empties_list->start == NULL || empties_list->end == NULL) {
		struct fibre_store_block *block;

		assert1(empties_list->start == NULL);
		assert1(empties_list->end == NULL);

		block = fibre_store_block_create(store->alloc);
		empties_list->start = &block->nodes[0];
		empties_list->end = &block->nodes[FIBRE_STORE_NODES_PER_BLOCK - 1];
	}

	/* assert that the list is non-empty */
	assert1(empties_list->start != NULL);
	assert1(empties_list->end != NULL);

	/* these should just generally be true of *every* list */
	assert1(empties_list->start->prev == NULL);
	assert1(empties_list->end->next == NULL);

	node = empties_list->start;

	if (node->next == NULL) {
		assert1(empties_list->end == empties_list->start);
		empties_list->start = empties_list->end = NULL;
	} else {
		assert1(empties_list->end != empties_list->start);
		node->next->prev = NULL;
		empties_list->start = node->next;
		node->next = NULL;
	}

	return &node->fibre;
}

static
struct fibre *
fibre_store_get_next_ready(struct fibre_store *store)
{
	struct fibre *fibre;
	size_t i;

	for (i = 0; i < FP_NUM_PRIOS; i++) {
		if (NULL != (fibre = fibre_store_list_dequeue(&store->lists[i]))) {
			return fibre;
		}
	}

	return NULL;
}

static struct fibre *current_fibre;
static struct fibre *main_fibre;
static struct fibre_store global_fibre_store;

void
fibre_init(struct alloc *alloc, size_t stack_size)
{
	size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
	size_t i;

	log_info("fibre", "Initialising fibre system with %lluMiB-sized stacks\n",
	                  stack_size / 1024 / 1024);

	if (sizeof(struct fibre_store_block) > page_size) {
		log_warning("fibre", "fibre_store_block is too big to fit in a page: lower FIBRE_STORE_NODES_PER_BLOCK (%llu)\n", sizeof(struct fibre_store_block));
	} else if (sizeof(struct fibre_store_block) + sizeof(struct fibre_store_node) <= page_size) {
		log_notice("fibre", "fibre_store_block could be bigger: raise FIBRE_STORE_NODES_PER_BLOCK (%llu)\n", sizeof(struct fibre_store_block));
	} else {
		log_info("fibre", "fibre_store_block fits perfectly in a page (%llu)\n", sizeof(struct fibre_store_block));
	}

	global_fibre_store.stack_size = stack_size;
	global_fibre_store.alloc = alloc;

	for (i = 0; i <= FP_NUM_PRIOS; i++) {
		fibre_store_list_init(&global_fibre_store.lists[i]);
	}

	current_fibre = main_fibre = fibre_store_get_first_empty(&global_fibre_store);
	main_fibre->state = FS_ACTIVE;
	main_fibre->prio = FP_NORMAL; /* is this right? */
	main_fibre->stack = NULL;
}

void
fibre_finish(void)
{
	log_info("fibre", "Deinitialising fibre system\n");

	log_info("fibre", "Fibre stat stack_allocs: %lld\n", fibre_stats.stack_allocs);
	log_info("fibre", "Fibre stat fibre_go_calls: %lld\n", fibre_stats.fibre_go_calls);

	/* TODO: need to somehow deallocate all the stacks */
	/* TODO: need to somehow deallocate all the fibre_store_blocks */
}

void
fibre_return(int ret)
{
	log_debug("fibre", "Returning from fibre %p with value %d\n",
	                   current_fibre, ret);

	if (current_fibre != main_fibre) {
		current_fibre->state = FS_EMPTY;
		fibre_store_list_enqueue(&global_fibre_store.lists[FP_NUM_PRIOS], current_fibre);
		/* TODO: When we finish with a fibre, we should mark its stack
		 * as no longer needed.  For now we will deallocate it, but
		 * constantly allocating and deallocating stacks might be
		 * wasteful.  It it does turn out to be, then we should reuse
		 * stacks.  However, reusing stacks has the potential for
		 * information leakage.  MADV_DONTNEED should zero-fill the
		 * pages if they're ever reused, however.  We need an
		 * allocator-dependent equivalent of:
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

static
void
fibre_enqueue(struct fibre *fibre)
{
	fibre_store_list_enqueue(&global_fibre_store.lists[fibre->prio], fibre);
}

int
fibre_yield(void)
{
	struct fibre_ctx *old, *new;
	struct fibre *old_fibre;
	struct fibre *fibre = fibre_store_get_next_ready(&global_fibre_store);

	if (fibre == NULL) {
		log_debug("fibre", "Yielding from fibre %p, finishing\n", current_fibre);
		return 0;
	}

	if (current_fibre->state != FS_EMPTY) {
		current_fibre->state = FS_READY;
		fibre_enqueue(current_fibre);
	}
	fibre->state = FS_ACTIVE;
	old = &current_fibre->ctx;
	new = &fibre->ctx;
	old_fibre = current_fibre;
	current_fibre = fibre;
	log_debug("fibre", "Yielding from fibre %p to fibre %p.\n", old_fibre, current_fibre);
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

	fibre_stats.fibre_go_calls++;
	if (fibre->stack == NULL) {
		fibre_stats.stack_allocs++;
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
	fibre->prio = FP_NORMAL;
	fibre->stack = stack;
	fibre_enqueue(fibre);
}
