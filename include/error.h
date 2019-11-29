/* The way error conditions work in vine is on the face of things quite simple.
 * There are two types of error condition: recoverable and unrecoverable.
 * Unrecoverable error conditions are simpler to deal with, so they will be
 * discussed first.
 *
 * Unrecoverable error conditions are logical errors in code: bugs. Failed
 * assertions, null dereferences, out-of-bounds errors, numerical underflow and
 * overflow, stack overflow, etc. These should result in abandonment of the
 * current process. That makes it especially critical to avoid them in C code,
 * because for the most part abandonment in the parts of vine written in C
 * means abandonment of the entire vine process.
 *
 * Recoverable error conditions are different. A recoverable error condition is
 * a particular subset of conditions in general. A condition is a state that
 * can be *signalled* by the running process. This prompts the runtime to look
 * at its (dynamically-scoped) stack of condition handlers. Condition handlers
 * are checked in order to find one that will handle the condition. When it
 * does, that handler is run without unwinding the stack. This is in stark
 * contrast to how exception handling works in typical languages, where the
 * search for a handler and the unwinding of the stack are interleaved. Here,
 * handlers are just executed like any other function, at the top of the stack,
 * with the same state and context as the code that signalled the condition.
 * Then this code can choose whether or not it wants to unwind the stack to
 * deal with the condition. If there is no handler to unwind the stack, then it
 * is treated like an unrecoverable error condition: in debug mode execution
 * switches to the debugger, while otherwise it causes the current process to
 * crash.
 */
