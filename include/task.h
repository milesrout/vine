/* Processes in vine are called 'tasks'. Tasks are independent threads of
 * execution.
 *
 *
 * Interoperation with operating system threads:
 * =============================================
 *
 * By default, tasks are not mapped in any particular way onto operating
 * system threads. The N operating system threads that vine is running on are
 * used in the most efficient available way to service the needs of M tasks.
 *
 * However, sometimes tasks need to know that they can stay on one or more
 * particular operating system thread. For example, they might need to retain
 * access to some operating system thread-local storage for interoperability
 * reasons. If this situation arises, they can be 'mapped' to a particular
 * fixed set of threads.
 *
 *
 * Synchronisation:
 * ================
 *
 * The only way for two tasks to synchronise is for one process to send
 * another process an (asynchronous) message. That's the only way. Just one,
 * exactly one. Go to Jail, do not pass Go(lang), do not collect $200.
 *
 *
 * Cancellation:
 * =============
 *
 * It's fucking annoying to be unable to press Ctrl-C to instantly cancel
 * tasks that are slow, unresponsive or simply no longer necessary. Most
 * threadlet systems are very bad at handling cancellation, in large parts
 * thanks to the appalling semantics of Unix signals in a parallel world.
 * Tasks are built from the ground up with an an understanding that dealing
 * with cancellation is necessary.
 *
 * (More explanation of cancel scopes and timeouts...)
 *
 */
