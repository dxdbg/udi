/*
 * Copyright (c) 2011-2013, Dan McNulty
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the UDI project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Thread support for pthreads

#include <inttypes.h>
#include <unistd.h>
#include <errno.h>

#include "udi.h"
#include "udirt.h"
#include "udirt-posix.h"
#include "udi-common.h"
#include "udi-common-posix.h"

// pthread function types and definitions

// these need to be weak to avoid linking pthreads with 
// a target that doesn't link pthreads

extern int pthread_sigmask(int how, const sigset_t *new_set, sigset_t *old_set) __attribute__((weak));
extern pthread_t pthread_self() __attribute__((weak));

// the threads list
static int num_threads = 0;
static thread *threads = NULL;

// event breakpoints
static breakpoint *thread_create_bp = NULL;
static breakpoint *thread_death_bp = NULL;

int THREAD_SUSPEND_SIGNAL = SIGUSR1;

/**
 * Determine if the debuggee is multithread capable (i.e., linked
 * against pthreads).
 * 
 * @return non-zero if the debuggee is multithread capable
 */
inline
int get_multithread_capable() {
    return pthread_sigmask != 0;
}

/**
 * Determine if the debuggee is multithreaded
 *
 * @return non-zero if the debuggee is multithread capable
 */
inline
int get_multithreaded() {
    return get_multithread_capable() && get_num_threads() > 1;
}

/**
 * Sets the signal mask for all threads in this process
 *
 * @return 0, on success; non-zero on failure
 */
int setsigmask(int how, const sigset_t *new_set, sigset_t *old_set) {
    // Only use pthread_sigmask when it is available
    if ( pthread_sigmask ) {
        return pthread_sigmask(how, new_set, old_set);
    }

    return sigprocmask(how, new_set, old_set);
}

/**
 * @return the user thread id for the currently executing thread
 */
uint64_t get_user_thread_id() {

    // pthread_self tends to be linked in single thread processes as well
    if (pthread_self &&  get_multithread_capable() ) {
        return (uint64_t)pthread_self();
    }

    return (uint64_t)UDI_SINGLE_THREAD_ID;
}

/**
 * Creates and installs a breakpoint at the specified address
 *
 * @param breakpoint_addr the breakpoint address
 * @param errmsg the error message populated on error
 *
 * @return the created breakpoint
 */
static
breakpoint *create_and_install(unsigned long breakpoint_addr, udi_errmsg *errmsg) {
    breakpoint *bp = create_breakpoint((udi_address)breakpoint_addr);

    if (bp == NULL) return NULL;

    if ( install_breakpoint(bp, errmsg) ) return NULL;

    return bp;
}

/**
 * Installs the thread event breakpoints
 *
 * @param errmsg the error message populated on failure
 *
 * @return 0 on success; non-zero otherwise
 */
int install_thread_event_breakpoints(udi_errmsg *errmsg) {

    if ( !get_multithread_capable() ) {
        snprintf(errmsg->msg, errmsg->size, "%s", "Process is not multithread capable");
        udi_printf("%s\n", errmsg->msg);
        return -1;
    }

    if (!pthreads_create_event || !pthreads_death_event) {
        snprintf(errmsg->msg, errmsg->size, "%s", "Failed to locate thread event functions");
        udi_printf("%s\n", errmsg->msg);
        return -1;
    }

    thread_create_bp = create_and_install((unsigned long)pthreads_create_event, errmsg);
    if ( thread_create_bp == NULL ) {
        udi_printf("%s\n", "failed to install thread create breakpoint");
        return -1;
    }

    thread_death_bp = create_and_install((unsigned long)pthreads_death_event, errmsg);
    if ( thread_death_bp == NULL ) {
        udi_printf("%s\n", "failed to install thread death breakpoint");
        return -1;
    }

    udi_printf("thread_create = 0x%"PRIx64" thread_death = 0x%"PRIx64"\n",
            thread_create_bp->address, thread_death_bp->address);

    return 0;
}

/**
 * @param bp the breakpoint
 *
 * @return non-zero if the specified breakpoint is a thread event breakpoint
 */
int is_thread_event_breakpoint(breakpoint *bp) {
    if ( thread_create_bp == bp ||
         thread_death_bp == bp ) return 1;

    return 0;
}

/**
 * @return The number of threads in this process
 */
inline
int get_num_threads() {
    return num_threads;
}

/**
 * Creates a thread structure for the specified thread id
 *
 * @return the created thread structure or NULL if it could not be created
 */
static
thread *create_thread(uint64_t tid) {

    int control_pipe[2];
    if ( get_multithread_capable() ) {
        // Create the pipes for synchronizing signal handler access
        if ( pipe(control_pipe) != 0 ) {
            udi_printf("failed to create sync pipe: %s\n", strerror(errno));
            return NULL;
        }
    }else{
        control_pipe[0] = -1;
        control_pipe[1] = -1;
    }

    // find the end of the list
    thread *cur_thread = threads;
    thread *last_thread = threads;

    while (cur_thread != NULL) {
        last_thread = cur_thread;
        cur_thread = cur_thread->next_thread;
    }

    thread *new_thr = (thread *)udi_malloc(sizeof(thread));
    if (new_thr == NULL) {
        close(control_pipe[0]);
        close(control_pipe[1]);
        return NULL;
    }

    new_thr->id = tid;
    new_thr->alive = 0;
    new_thr->dead = 0;
    new_thr->request_handle = -1;
    new_thr->response_handle = -1;
    new_thr->next_thread = NULL;
    new_thr->control_read = control_pipe[0];
    new_thr->control_write = control_pipe[1];
    new_thr->ts = UDI_TS_RUNNING;
    num_threads++;

    if (last_thread == NULL) {
        threads = new_thr;
    }else{
        last_thread->next_thread = new_thr;
    }

    return new_thr;
}

/**
 * Destroys the thread structure with the specified thread id, does nothing if no thread
 * exists with the specified thread id
 */
static
void destroy_thread(uint64_t tid) {

    thread *thr = threads;
    thread *last_thread = threads;
    while (thr != NULL) {
        if (thr->id == tid) break;

        last_thread = thr;
        thr = thr->next_thread;
    }

    if (thr == NULL) return;

    if (thr == threads) {
        threads = NULL;
    }else{
        last_thread->next_thread = thr->next_thread;
    }

    close(thr->control_write);
    close(thr->control_read);

    udi_free(thr);
    num_threads--;
}

/**
 * @param tid the thread id
 *
 * @return the thread or NULL if none could be found
 */
static
thread *find_thread(uint64_t tid) {
    thread *thr = threads;
    while (thr != NULL) {
        if (thr->id == tid) break;
        thr = thr->next_thread;
    }

    return thr;
}

/**
 * @return the head of the thread list
 */
thread *get_thread_list() {
    return threads;
}

/**
 * @return the current thread
 */
thread *get_current_thread() {
    uint64_t tid = get_user_thread_id();

    thread *thr = threads;
    while (thr != NULL) {
        if (thr->id == tid) break;
        thr = thr->next_thread;
    }

    return thr;
}

/**
 * Creates the initial thread
 *
 * @return the created thread or null if there was an error
 */
thread *create_initial_thread() {

    thread *thr = create_thread(get_user_thread_id());

    // the initial thread is already alive
    thr->alive = 1;
    return thr;
}

/**
 * Handle the thread create event
 *
 * @param context the context
 * @param errmsg the error message populated on error
 * @param errmsg_size the maximum size of the error message
 */
static
event_result handle_thread_create(const ucontext_t *context, udi_errmsg *errmsg) {
    event_result result;
    result.wait_for_request = 1;
    result.failure = 0;

    uint64_t tid;
    do {
        tid = initialize_thread(errmsg);
        if ( tid == 0 ) {
            result.failure = 1;
            break;
        }

        udi_printf("thread create event for 0x%"PRIx64"\n", tid);

        thread *thr = create_thread(tid);
        if (thr == NULL) {
            result.failure = 1;
            break;
        }

        if ( thread_create_callback(thr, errmsg) != 0 ) {
            result.failure = 1;
            break;
        }

        udi_event_internal event = create_event_thread_create(tid);
        result.failure = write_event(&event);

        if ( thread_create_handshake(thr, errmsg) != 0 ) {
            result.failure = 1;
            break;
        }
    }while(0);

    if ( result.failure ) {
        udi_printf("failed to report thread create of 0x%"PRIx64"\n", tid);
    }

    return result;
}

/**
 * Handle the thread death event
 *
 * @param context the context
 * @param errmsg the error message populated on error
 */
static
event_result handle_thread_death(const ucontext_t *context, udi_errmsg *errmsg) {
    event_result result;
    result.wait_for_request = 1;
    result.failure = 0;

    uint64_t tid;
    do {
        tid = finalize_thread(errmsg);

        if (tid == 0) {
            result.failure = 1;
            break;
        }
    
        udi_printf("thread death event for 0x%"PRIx64"\n", tid);

        thread *thr = find_thread(tid);
        if (thr == NULL) {
            result.failure = 1;
            break;
        }

        if ( thread_death_callback(thr, errmsg) != 0 ) {
            result.failure = 1;
            break;
        }

        destroy_thread(tid);

        udi_event_internal event = create_event_thread_death(tid);
        result.failure = write_event(&event);
    }while(0);

    if ( result.failure ) {
        udi_printf("failed to report thread death of 0x%"PRIx64"\n", tid);
    }

    return result;
}

/**
 * @param bp the breakpoint
 * @param context the context
 * @param errmsg the error message populated on error
 */
event_result handle_thread_event_breakpoint(breakpoint *bp, const ucontext_t *context,
        udi_errmsg *errmsg)
{
    if (bp == thread_create_bp) {
        return handle_thread_create(context, errmsg);
    }

    if (bp == thread_death_bp) {
        return handle_thread_death(context, errmsg);
    }

    event_result err_result;
    err_result.failure = 1;
    err_result.wait_for_request = 0;

    snprintf(errmsg->msg, errmsg->size, "failed to handle the thread event at 0x%"PRIx64,
            bp->address);
    udi_printf("%s\n", errmsg->msg); 

    return err_result;
}
