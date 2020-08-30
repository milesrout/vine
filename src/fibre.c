#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "alloc.h"
#include "fibre.h"
#include "printf.h"
#include "abort.h"
#include "log.h"
#include "memory.h"

#ifdef VINE_USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "fibre_switch.h"

static
struct {
	long int fstat_stack_allocs;
	long int fstat_fibre_go_calls;
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
	uint64_t fc_rsp;
	uint64_t fc_r15;
	uint64_t fc_r14;
	uint64_t fc_r13;
	uint64_t fc_r12;
	uint64_t fc_rbx;
	uint64_t fc_rbp;
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
	struct fibre_ctx f_ctx;
	short f_state;
	short f_prio;
	unsigned f_valgrind_id;
	char *f_stack;
};

static
void
fibre_setup(struct fibre *fibre)
{
	/* f_ctx is invalid when f_state = EMPTY */
	fibre->f_state = FS_EMPTY;
	/* f_prio is invalid when f_state = EMPTY */
	/* f_valgrind_id is initialised when needed */
	fibre->f_stack = NULL;
}

/*
 * This is a node in an intrusive bidirectional linked list of struct fibres.
 */
struct fibre_store_node {
	struct fibre_store_node *fsn_prev, *fsn_next;
	struct fibre fsn_fibre;
};

/*
 * This is the control block for a linked list of struct fibres.
 * it has two valid states:
 *   fsl_start===NULL & fsl_end===NULL -> list is empty
 *   fsl_start=/=NULL & fsl_end=/=NULL -> list is non-empty
 */
struct fibre_store_list {
	struct fibre_store_node *fsl_start, *fsl_end;
};

static
void
fibre_store_list_init(struct fibre_store_list *list)
{
	list->fsl_start = list->fsl_end = NULL;
}

/* adds a fibre to the start of the list */
static
void
fibre_store_list_enqueue(struct fibre_store_list *list, struct fibre *fibre)
{
	struct fibre_store_node *node =
		container_of(struct fibre_store_node, fsn_fibre, fibre);

	/* require that the fibre is not already in a list */
	assert1(node->fsn_next == NULL);
	assert1(node->fsn_prev == NULL);

	if (list->fsl_start == NULL || list->fsl_end == NULL) {
		assert1(list->fsl_start == NULL);
		assert1(list->fsl_end == NULL);

		list->fsl_start = node;
		list->fsl_end = node;
	} else {
		assert1(list->fsl_start != NULL);
		assert1(list->fsl_end != NULL);

		list->fsl_start->fsn_prev = node;
		node->fsn_next = list->fsl_start;
		list->fsl_start = node;
	}
}

/* removes a fibre from the end of the list */
static
struct fibre *
try_fibre_store_list_dequeue(struct fibre_store_list *list)
{
	struct fibre_store_node *node;

	if (list->fsl_start == NULL || list->fsl_end == NULL) {
		assert1(list->fsl_start == NULL);
		assert1(list->fsl_end == NULL);

		return NULL;
	}

	/* require that the list is non-empty */
	assert1(list->fsl_start != NULL);
	assert1(list->fsl_end != NULL);

	/* these should just generally be true of *every* list */
	assert1(list->fsl_start->fsn_prev == NULL);
	assert1(list->fsl_end->fsn_next == NULL);

	node = list->fsl_end;

	if (node->fsn_prev == NULL) {
		assert1(list->fsl_end == list->fsl_start);
		list->fsl_start = list->fsl_end = NULL;
	} else {
		assert1(list->fsl_end != list->fsl_start);
		node->fsn_prev->fsn_next = NULL;
		list->fsl_end = node->fsn_prev;
		node->fsn_prev = NULL;
	}

	return &node->fsn_fibre;
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
	struct fibre_store_block *fsb_next;
	struct fibre_store_node fsb_nodes[FIBRE_STORE_NODES_PER_BLOCK];
};

/*
 * there is a list of ready fibres for each priority level plus a list of
 * empty fibres.
 */
struct fibre_store {
	size_t fs_stack_size;
	struct alloc *fs_alloc;
	struct fibre_store_block *fs_blocks;
	/* the last list is for FS_EMPTY fibres */
	struct fibre_store_list fs_lists[FP_NUM_PRIOS + 1];
};

static
struct fibre_store_block *
fibre_store_block_create(struct fibre_store *store)
{
	struct fibre_store_block *block =
		allocate_with(store->fs_alloc, sizeof *block);
	size_t i;

#define MAX (FIBRE_STORE_NODES_PER_BLOCK - 1)
	/* set up the nodes in the block to form a bidirectional linked list */
	for (i = 0; i <= MAX; i++) {
		block->fsb_nodes[i].fsn_prev = (i == 0) ? NULL : &block->fsb_nodes[i - 1];
		block->fsb_nodes[i].fsn_next = (i == MAX) ? NULL : &block->fsb_nodes[i + 1];
		fibre_setup(&block->fsb_nodes[i].fsn_fibre);
	}
#undef MAX
	
	block->fsb_next = store->fs_blocks;
	store->fs_blocks = block;

	return block;
}

/*
 * This takes fibres from the start of the list instead of the end, treating the
 * empties list as a stack. The other lists are treated as queues.
 */
static
struct fibre *
fibre_store_get_first_empty(struct fibre_store *store)
{
	struct fibre_store_list *empties_list = &store->fs_lists[FP_NUM_PRIOS];
	struct fibre_store_node *node;

	if (empties_list->fsl_start == NULL || empties_list->fsl_end == NULL) {
		struct fibre_store_block *block;

		assert1(empties_list->fsl_start == NULL);
		assert1(empties_list->fsl_end == NULL);

		block = fibre_store_block_create(store);
		empties_list->fsl_start = &block->fsb_nodes[0];
		empties_list->fsl_end =
			&block->fsb_nodes[FIBRE_STORE_NODES_PER_BLOCK - 1];
	}

	/* assert that the list is non-empty */
	assert1(empties_list->fsl_start != NULL);
	assert1(empties_list->fsl_end != NULL);

	/* these should just generally be true of *every* list */
	assert1(empties_list->fsl_start->fsn_prev == NULL);
	assert1(empties_list->fsl_end->fsn_next == NULL);

	node = empties_list->fsl_start;

	if (node->fsn_next == NULL) {
		assert1(empties_list->fsl_end == empties_list->fsl_start);
		empties_list->fsl_start = empties_list->fsl_end = NULL;
	} else {
		assert1(empties_list->fsl_end != empties_list->fsl_start);
		node->fsn_next->fsn_prev = NULL;
		empties_list->fsl_start = node->fsn_next;
		node->fsn_next = NULL;
	}

	return &node->fsn_fibre;
}

static
struct fibre *
fibre_store_get_next_ready(struct fibre_store *store)
{
	struct fibre *fibre;
	size_t i;

	for (i = 0; i < FP_NUM_PRIOS; i++) {
		fibre = try_fibre_store_list_dequeue(&store->fs_lists[i]);
		if (fibre != NULL) {
			return fibre;
		}
	}

	return NULL;
}

static
struct fibre
*current_fibre, *main_fibre;

static
struct fibre_store
global_fibre_store;


void
fibre_init(struct alloc *alloc, size_t stack_size)
{
	size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
	size_t i;

	log_info("fibre",
		"Initialising fibre system with %luMiB-sized stacks\n",
	        stack_size / 1024 / 1024);

#define BLOCK_SIZE (sizeof(struct fibre_store_block))
#define NODE_SIZE (sizeof(struct fibre_store_node))
	if (BLOCK_SIZE > page_size) {
		log_warning("fibre",
			"fibre_store_block is too big to fit in a page: "
			"lower FIBRE_STORE_NODES_PER_BLOCK (%lu)\n",
			BLOCK_SIZE);
	} else if (BLOCK_SIZE + NODE_SIZE <= page_size) {
		log_notice("fibre",
			"fibre_store_block could be bigger: "
			"raise FIBRE_STORE_NODES_PER_BLOCK (%lu)\n",
			BLOCK_SIZE);
	} else {
		log_info("fibre",
			"fibre_store_block fits perfectly in a page (%lu)\n",
			BLOCK_SIZE);
	}
#undef BLOCK_SIZE
#undef NODE_SIZE

	global_fibre_store.fs_stack_size = stack_size;
	global_fibre_store.fs_alloc = alloc;
	global_fibre_store.fs_blocks = NULL;

	/* not a bug: "<=" because there is one more list than priority */
	for (i = 0; i <= FP_NUM_PRIOS; i++) {
		fibre_store_list_init(&global_fibre_store.fs_lists[i]);
	}

	main_fibre = fibre_store_get_first_empty(&global_fibre_store);
	main_fibre->f_state = FS_ACTIVE;
	main_fibre->f_prio = FP_NORMAL; /* is this right? */
	main_fibre->f_valgrind_id = 0; /* see valgrind/m_stacks.c/next_id */
	main_fibre->f_stack = NULL;

	current_fibre = main_fibre;
}

/* This should be further up, but the assertion requires it to be after the
 * declaration of main_fibre */
static
void
fibre_store_destroy(struct fibre_store *store)
{
	size_t i;
	while (store->fs_blocks != NULL) {
		struct fibre_store_block *next = store->fs_blocks->fsb_next;
		for (i = 0; i < FIBRE_STORE_NODES_PER_BLOCK; i++) {
			if (&store->fs_blocks->fsb_nodes[i].fsn_fibre == main_fibre)
				continue;
			if (store->fs_blocks->fsb_nodes[i].fsn_fibre.f_stack == NULL)
				continue;
			deallocate_with(store->fs_alloc,
				store->fs_blocks->fsb_nodes[i].fsn_fibre.f_stack,
				store->fs_stack_size);
		}
		deallocate_with(store->fs_alloc,
			store->fs_blocks,
			sizeof *store->fs_blocks);
		store->fs_blocks = next;
	}
}


void
fibre_finish(void)
{
	log_info("fibre", "Deinitialising fibre system\n");

	log_info("fibre",
		"Fibre stat stack_allocs: %ld\n", fibre_stats.fstat_stack_allocs);
	log_info("fibre",
		"Fibre stat fibre_go_calls: %ld\n", fibre_stats.fstat_fibre_go_calls);

	fibre_store_destroy(&global_fibre_store);
}

void
fibre_return(void)
{
	log_debug("fibre", "Returning from fibre %p\n", (void *)current_fibre);

	if (current_fibre != main_fibre) {
		current_fibre->f_state = FS_EMPTY;
		fibre_store_list_enqueue(
			&global_fibre_store.fs_lists[FP_NUM_PRIOS],
			current_fibre);
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
#ifdef VINE_USE_VALGRIND
		VALGRIND_STACK_DEREGISTER(current_fibre->f_valgrind_id);
#endif
		fibre_yield();
		/* unreachable */
	}
	while (fibre_yield())
		;
	fibre_finish();
}

static
void
fibre_enqueue(struct fibre *fibre)
{
	fibre_store_list_enqueue(&global_fibre_store.fs_lists[fibre->f_prio], fibre);
}

int
fibre_yield(void)
{
	struct fibre_ctx *old, *new;
	struct fibre *old_fibre;
	struct fibre *fibre = fibre_store_get_next_ready(&global_fibre_store);

	if (fibre == NULL) {
		log_debug("fibre", "Yielding from fibre %p, finishing\n",
		                   (void *)current_fibre);
		return 0;
	}

	if (current_fibre->f_state != FS_EMPTY) {
		current_fibre->f_state = FS_READY;
		fibre_enqueue(current_fibre);
	}
	fibre->f_state = FS_ACTIVE;
	old = &current_fibre->f_ctx;
	new = &fibre->f_ctx;
	old_fibre = current_fibre;
	current_fibre = fibre;
	log_debug("fibre", "Yielding from fibre %p to fibre %p.\n",
	                   (void *)old_fibre,
	                   (void *)current_fibre);
#ifdef VINE_USE_VALGRIND
	if (current_fibre != main_fibre) {
		current_fibre->f_valgrind_id = VALGRIND_STACK_REGISTER(
			current_fibre->f_stack,
			current_fibre->f_stack + global_fibre_store.fs_stack_size);
	}
#endif
	fibre_switch(old, new);
#ifdef VINE_USE_VALGRIND
	if (old_fibre != main_fibre) {
		VALGRIND_STACK_DEREGISTER(old_fibre->f_valgrind_id);
	}
#endif
	return 1;
}

#define FMTREG "0x%010llx"

static void
fibre_stop(void)
{
	fibre_return();
}

void
/* fibre_go(void (*f)(void *), void *data) */
fibre_go(void (*f)(void))
{
	char *stack;
	size_t size = global_fibre_store.fs_stack_size;
	struct fibre *fibre = fibre_store_get_first_empty(&global_fibre_store);

	fibre_stats.fstat_fibre_go_calls++;
	if (fibre->f_stack == NULL) {
		fibre_stats.fstat_stack_allocs++;
		stack = allocate_with(global_fibre_store.fs_alloc, size);
	} else {
		stack = fibre->f_stack;
	}
#ifdef VINE_USE_VALGRIND
	fibre->f_valgrind_id = VALGRIND_STACK_REGISTER(stack, stack + size);
#endif

	*(uint64_t *)&stack[size -  8] = (uint64_t)fibre_stop;
	*(uint64_t *)&stack[size - 16] = (uint64_t)f;
	fibre->f_ctx.fc_rsp = (uint64_t)&stack[size - 16];
	/* Obviously this doesn't actually work! OBVIOUSLY! We need to use an
	 * assembly function to put this pointer into rsi before the first call
	 * to this fibre.
	 *     fibre->f_ctx.rsi = (uint64_t)data;
	 */
	fibre->f_state = FS_READY;
	fibre->f_prio = FP_NORMAL;
	fibre->f_stack = stack;
	fibre_enqueue(fibre);
}
