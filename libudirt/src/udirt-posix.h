/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// UDI debuggee implementation common between all platforms

#ifndef _UDI_RT_POSIX_H
#define _UDI_RT_POSIX_H 1

// This needs to be included first to set feature macros
#include "udirt-platform.h"

#include <ucontext.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <dlfcn.h>

#include "udirt.h"

#ifdef __cplusplus
extern "C" {
#endif

// syscall events
typedef int (*sigaction_type)(int, const struct sigaction *, 
        struct sigaction *);
typedef pid_t (*fork_type)(void);
typedef int (*execve_type)(const char *, char *const *, char *const *);
typedef void (* (*signal_type)(int, void (*handler)(int)))(int);

extern sigaction_type real_sigaction;
extern fork_type real_fork;
extern execve_type real_execve;

int locate_wrapper_functions(udi_errmsg *errmsg);
int locate_thread_wrapper_functions(udi_errmsg *errmsg);
int install_event_breakpoints(udi_errmsg *errmsg);

/**
 * Wait for and request and process the request on reception.
 *
 * @param errmsg error message populate on error
 * @param thr populated by the thread that received the command
 *
 * @return request handler return code
 */
int wait_and_execute_command(udi_errmsg *errmsg, thread **thr);

// re-initialize a process after fork
void reinit_udirt();

// exit event handling

/** indicates that the process will exit on the next continue request */
extern int exiting;

/**
 * Determines the argument to the exit function, given the context at which the exit breakpoint
 * was hit
 *
 * @param context the current context
 * @param errmsg the error message populated by the memory access
 *
 * @return the result
 */
int get_exit_argument(const ucontext_t *context, int *status, udi_errmsg *errmsg);

// library wrapping
extern void *UDI_RTLD_NEXT;

// breakpoint handling //

/**
 * Given the context, calculates the address at which a trap occurred at.
 *
 * @param context the context containing the current PC value
 *
 * @return the computed address
 */
uint64_t get_trap_address(const ucontext_t *context);

/**
 * Given the context, sets the pc to the supplied value
 *
 * @param context the context containing the current PC value
 * @param pc the new pc value
 */
void set_pc(ucontext_t *context, unsigned long pc);

// signal handling
int setup_signal_handlers();
int uninstall_signal_handlers();
void app_signal_handler(int signal, siginfo_t *siginfo, void *v_context);
void signal_entry_point(int signal, siginfo_t *siginfo, void *v_context);

// write failure handling
extern int pipe_write_failure;

// pthreads support //

// signal handling
typedef struct signal_state_struct {
    int signal;
    siginfo_t siginfo;
    ucontext_t context;
    void *context_data;
    int context_valid;
} signal_state;

extern int THREAD_SUSPEND_SIGNAL;

struct thread_struct {
    uint64_t id;
    udi_thread_state_e ts;
    int dead;
    int request_handle;
    int response_handle;
    int control_write;
    int control_read;
    int control_thread;
    int suspend_pending;
    int single_step;
    int stack_event_pending;
    breakpoint *single_step_bp;
    signal_state event_state;
    struct thread_struct *next_thread;
};

int setsigmask(int how, const sigset_t *new_set, sigset_t *old_set);

/**
 * Allocate the platform-specific context data
 *
 * @param context_data output parameter
 *
 * @return 0 on success; non-zero on failure
 */
int allocate_context_data(void **context_data);

/**
 * Copy the source ucontext_t to the target signal state
 *
 * @param src the source
 * @param dst the destination
 */
void copy_context(const ucontext_t *src, signal_state *dst);

/**
 * @return the kernel thread id for the currently executing thread
 */
uint64_t get_kernel_thread_id();

// thread synchronization
typedef struct udi_barrier_struct {
    unsigned int sync_var;
    int read_handle;
    int write_handle;
} udi_barrier;

/**
 * Initializes the thread synchronization mechanism
 *
 * @return 0 on success; non-zero on failure
 */
int initialize_thread_sync();

/**
 * Barrier used to synchronize actions with other threads
 */
extern udi_barrier thread_barrier;

/**
 * Value used as signal with thread_barrier and thread control pipe
 */
extern const unsigned char sentinel;

/**
 * The first thread that calls this function forces all other threads into the UDI signal handler,
 * which eventually routes to this function.
 *
 * If a thread isn't the first thread calling this function, it will block until the library
 * decides the thread should continue executing, which is a combination of the user's desired
 * run state for a thread and the library's desired run state for a thread.
 *
 * @return less than 0 if there is an error
 * @return 0 if the thread was the first thread or the thread should handle its event now
 * @return greater than 0 if the thread was not the first thread
 */
int block_other_threads();

/**
 * Releases the other threads that are blocked in block_other_threads
 *
 * Marks any threads that were created during this request handling
 *
 * @return 0 on success
 * @return non-zero on failure
 */
int release_other_threads();

/**
 * @return non-zero if a single thread will be executing due to a library operation (such as
 * a memory access to a non-accessible address or a single-step event)
 */
int single_thread_executing();

/**
 * Creates the initial thread
 *
 * @return the created thread or null if there was an error
 */
thread *create_initial_thread();

/**
 * Called by the thread support implementation
 *
 * @param thr the thread structure for the created thread
 * @param errmsg the error message
 *
 * @return 0 on success; non-zero on failure
 */
int thread_create_callback(thread *thr, udi_errmsg *errmsg);

/**
 * Performs the handshake with the debugger after the creation of the thread files
 *
 * @param thr the thread structure for the created thread
 * @param errmsg the error message
 *
 * @return 0 on success; non-zero on failure
 */
int thread_create_handshake(thread *thr, udi_errmsg *errmsg);

/**
 * Called by the thread support implementation before thread event is published
 *
 * @param thr the thread structure for the dead thread
 * @param errmsg the error message
 *
 * @return 0 on success; non-zero on failure
 */
int thread_death_callback(thread *thr, udi_errmsg *errmsg);

/**
 * Destroys the specified thread structure
 *
 * @param thr the thread
 */
void destroy_thread(thread *thr);

#ifdef __cplusplus
} // extern C
#endif

#endif
