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

// UDI debuggee implementation common between all platforms

#ifndef _UDI_RT_POSIX_H
#define _UDI_RT_POSIX_H 1

#include <ucontext.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <dlfcn.h>

#include "udirt.h"

#ifdef __cplusplus
extern "C" {
#endif

// event handling
typedef struct event_result_struct {
    int failure;
    int wait_for_request;
} event_result;

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
int install_event_breakpoints(udi_errmsg *errmsg);
int is_event_breakpoint(breakpoint *bp);
event_result handle_event_breakpoint(breakpoint *bp, const ucontext_t *context, udi_errmsg *errmsg);

int wait_and_execute_command(udi_errmsg *errmsg);

// re-initialize a process after fork
void reinit_udi_rt();

// exit event handling
typedef struct exit_result_struct {
    int status;
    int failure;
} exit_result;

exit_result get_exit_argument(const ucontext_t *context, udi_errmsg *errmsg);

// library wrapping
extern void *UDI_RTLD_NEXT;

// breakpoint handling
udi_address get_trap_address(const ucontext_t *context);
void rewind_pc(ucontext_t *context);

// context modification
void set_pc(ucontext_t *context, unsigned long pc);
unsigned long get_pc(const ucontext_t *context);

// signal handling

#define MAX_SIGNAL_NUM 31

// This is the number of elements in the signals array
#define NUM_SIGNALS 29

extern int signal_map[];
extern int signals[];
extern struct sigaction app_actions[];

int setup_signal_handlers();
void app_signal_handler(int signal, siginfo_t *siginfo, void *v_context);
void signal_entry_point(int signal, siginfo_t *siginfo, void *v_context);

// pthreads support //

// signal handling
typedef struct signal_state_struct {
    int signal;
    siginfo_t siginfo;
    ucontext_t context;
} signal_state;
extern int THREAD_SUSPEND_SIGNAL;

struct thread_struct {
    uint64_t id;
    udi_thread_state_e ts;
    int alive;
    int dead;
    int request_handle;
    int response_handle;
    int control_write;
    int control_read;
    int control_thread;
    int suspend_pending;
    signal_state event_state;
    struct thread_struct *next_thread;
};

int get_num_threads();
int get_multithread_capable();
int get_multithreaded();
int setsigmask(int how, const sigset_t *new_set, sigset_t *old_set);
thread *get_thread_list();
uint64_t get_user_thread_id();
uint32_t get_kernel_thread_id();
thread *get_current_thread();

// thread synchronization
typedef struct udi_barrier_struct {
    unsigned int sync_var;
    int read_handle;
    int write_handle;
} udi_barrier;
int initialize_thread_sync();
int block_other_threads();
int release_other_threads();

// thread events
extern void (*pthreads_create_event)(void);
extern void (*pthreads_death_event)(void);

int initialize_pthreads_support(udi_errmsg *errmsg);

int install_thread_event_breakpoints(udi_errmsg *errmsg);
int is_thread_event_breakpoint(breakpoint *bp);
event_result handle_thread_event_breakpoint(breakpoint *bp, const ucontext_t *context,
        udi_errmsg *errmsg);

thread *create_initial_thread();
int thread_create_callback(thread *thr, udi_errmsg *errmsg);
int thread_create_handshake(thread *thr, udi_errmsg *errmsg);
uint64_t initialize_thread(udi_errmsg *errmsg);

int thread_death_callback(thread *thr, udi_errmsg *errmsg);
uint64_t finalize_thread(udi_errmsg *errmsg);

#ifdef __cplusplus
} // extern C
#endif

#endif
