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

extern sigaction_type real_sigaction;
extern fork_type real_fork;
extern execve_type real_execve;

int locate_wrapper_functions(char *errmsg, unsigned int errmsg_size);
int install_event_breakpoints(char *errmsg, unsigned int errmsg_size);

// exit event handling
typedef struct exit_result_struct {
    int status;
    int failure;
} exit_result;

extern breakpoint *exit_bp;

int get_exit_inst_length(void (*exit_func)(int), char *errmsg, unsigned int errmsg_size);
exit_result get_exit_argument(const ucontext_t *context, char *errmsg, unsigned int errmsg_size);
event_result handle_exit_breakpoint(const ucontext_t *context, char *errmsg, unsigned int errmsg_size);

// breakpoint handling
udi_address get_trap_address(const ucontext_t *context);
void rewind_pc(ucontext_t *context);

// signal handling

// This is the number of elements in the signals array
#define NUM_SIGNALS 29

extern struct sigaction default_lib_action;
extern int signal_map[];
extern int signals[];
extern struct sigaction app_actions[];

int setup_signal_handlers();
void app_signal_handler(int signal, siginfo_t *siginfo, void *v_context);

// pthreads support
int get_multithread_capable();
int setsigmask(int how, const sigset_t *new_set, sigset_t *old_set);

#ifdef __cplusplus
} // extern C
#endif

#endif
