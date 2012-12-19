/*
 * Copyright (c) 2011-2012, Dan McNulty
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

static unsigned long DEFAULT_SINGLE_THREAD_ID = 0xDEADBEEF;

// event breakpoints
static breakpoint *thread_create_bp = NULL;
static breakpoint *thread_destroy_bp = NULL;

/**
 * Determine if the debuggee is multithread capable (i.e., linked
 * against pthreads).
 * 
 * @return non-zero if the debugee is multithread capable
 */
inline
int get_multithread_capable() {
    return pthread_sigmask != 0;
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
unsigned long get_user_thread_id() {

    // pthread_self tends to be linked in single thread processes as well
    if (pthread_self &&  get_multithread_capable() ) {
        return (unsigned long)pthread_self();
    }

    return DEFAULT_SINGLE_THREAD_ID;
}

/**
 * Creates and installs a breakpoint at the specified address
 *
 * @param breakpoint_addr the breakpoint address
 * @param errmsg the error message populated on error
 * @param errmsg_size the maximum size of the error message
 *
 * @return the created breakpoint
 */
static
breakpoint *create_and_install(unsigned long breakpoint_addr, char *errmsg, 
        unsigned int errmsg_size) 
{
    breakpoint *bp = create_breakpoint((udi_address)breakpoint_addr);

    if (bp == NULL) return NULL;

    if ( install_breakpoint(bp, errmsg, errmsg_size) ) return NULL;

    return bp;
}

/**
 * Installs the thread event breakpoints
 *
 * @param errmsg the error message populated on failure
 * @param errmsg_size the size of the error message
 *
 * @return 0 on success; non-zero otherwise
 */
int install_thread_event_breakpoints(char *errmsg, unsigned int errmsg_size) {

    if ( !get_multithread_capable() ) {
        snprintf(errmsg, errmsg_size, "%s", "Process is not multithread capable");
        udi_printf("%s\n", errmsg);
        return -1;
    }

    find_pthreads_breakpoints();

    if (!pthreads_create_event || !pthreads_death_event) {
        snprintf(errmsg, errmsg_size, "%s", "Failed to locate thread event functions");
        udi_printf("%s\n", errmsg);
        return -1;
    }

    thread_create_bp = create_and_install((unsigned long)pthreads_create_event, errmsg,
            errmsg_size);

    if ( thread_create_bp == NULL ) {
        udi_printf("%s\n", "failed to install thread create breakpoint");
        return -1;
    }

    thread_destroy_bp = create_and_install((unsigned long)pthreads_death_event, errmsg,
            errmsg_size);

    if ( thread_destroy_bp == NULL ) {
        udi_printf("%s\n", "failed to install thread destroy breakpoint");
        return -1;
    }

    udi_printf("thread_create = 0x%"PRIx64" thread_destroy = 0x%"PRIx64"\n",
            thread_create_bp->address, thread_destroy_bp->address);

    return 0;
}

/**
 * @param bp the breakpoint
 *
 * @return non-zero if the specified breakpoint is a thread event breakpoint
 */
int is_thread_event_breakpoint(breakpoint *bp) {
    if ( thread_create_bp == bp ||
         thread_destroy_bp == bp ) return 1;

    return 0;
}

/**
 * Handle the thread create event
 *
 * @param context the context
 * @param errmsg the error message populated on error
 * @param errmsg_size the maximum size of the error message
 */
static
event_result handle_thread_create(const ucontext_t *context,
        char *errmsg, unsigned int errmsg_size)
{
    event_result result;
    result.wait_for_request = 1;
    result.failure = 0;

    uint32_t tid = get_kernel_thread_id();

    udi_printf("thread create event for %x\n", tid);


    do {
        if ( thread_create_callback(tid, errmsg, errmsg_size) != 0 ) {
            result.failure = 1;
            break;
        }

        udi_event_internal event = create_event_thread_create(tid);
        if ( event.packed_data == NULL ) {
            result.failure = 1;
            break;
        }

        result.failure = write_event(&event);

        udi_free(event.packed_data);
    }while(0);

    if ( result.failure ) {
        udi_printf("failed to report thread create of %u\n", tid);
    }

    return result;
}

/**
 * Handle the thread destroy event
 *
 * @param context the context
 * @param errmsg the error message populated on error
 * @param errmsg_size the maximum size of the error message
 */
static
event_result handle_thread_destroy(const ucontext_t *context, char *errmsg,
        unsigned int errmsg_size)
{
    event_result result;
    result.wait_for_request = 1;
    result.failure = 0;

    uint32_t tid = get_kernel_thread_id();

    udi_printf("thread destroy event for %u\n", tid);

    do {
        if ( thread_destroy_callback(tid, errmsg, errmsg_size) != 0 ) {
            result.failure = 1;
            break;
        }

        udi_event_internal event = create_event_thread_destroy(tid);
        if ( event.packed_data == NULL ) {
            result.failure = 1;
            break;
        }

        result.failure = write_event(&event);

        udi_free(event.packed_data);
    }while(0);

    if ( result.failure ) {
        udi_printf("failed to report thread destroy of %u\n", tid);
    }

    return result;
}

/**
 * @param bp the breakpoint
 * @param context the context
 * @param errmsg the error message populated on error
 * @param errmsg_size the maximum size of the error message
 */
event_result handle_thread_event_breakpoint(breakpoint *bp, const ucontext_t *context,
        char *errmsg, unsigned int errmsg_size)
{
    if (bp == thread_create_bp) {
        return handle_thread_create(context, errmsg, errmsg_size);
    }

    if (bp == thread_destroy_bp) {
        return handle_thread_destroy(context, errmsg, errmsg_size);
    }

    event_result err_result;
    err_result.failure = 1;
    err_result.wait_for_request = 0;

    snprintf(errmsg, errmsg_size, "failed to handle the thread event at 0x%"PRIx64,
            bp->address);
    udi_printf("%s\n", errmsg); 

    return err_result;
}
