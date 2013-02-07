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

#ifndef _LIBUDI_H
#define _LIBUDI_H 1

#include "udi.h"

#ifdef __cplusplus
extern "C" {
#endif

// Note: functions comments are with the implementation

// Opaque process handle
typedef struct udi_process_struct udi_process;

// Opaque thread handle
typedef struct udi_thread_struct udi_thread;

/**
 * library error codes
 */
typedef enum {
    UDI_ERROR_LIBRARY, /// there was an internal library error
    UDI_ERROR_REQUEST, /// the request was invalid
    UDI_ERROR_NONE
} udi_error_e;

// Global state functions //
const char *get_error_message(udi_error_e error_code);
udi_error_e init_libudi();
udi_error_e set_udi_root_dir(const char *root_dir);

// Process management //
udi_process *create_process(const char *executable, char * const argv[],
        char * const envp[]);
udi_error_e free_process(udi_process *proc);
udi_error_e continue_process(udi_process *proc);
udi_error_e refresh_state(udi_process *proc);

// Process properties //
void set_user_data(udi_process *proc, void *user_data);
void *get_user_data(udi_process *proc);
int get_proc_pid(udi_process *proc);
udi_arch_e get_proc_architecture(udi_process *proc);
int get_multithread_capable(udi_process *proc);
udi_thread *get_initial_thread(udi_process *proc);

// Thread properties //
uint64_t get_tid(udi_thread *thr);
udi_process *get_process(udi_thread *thr);
udi_thread_state_e get_state(udi_thread *thr);
udi_thread *get_next_thread(udi_thread *thr);

// Breakpoint interface //
udi_error_e create_breakpoint(udi_process *proc, udi_address addr);
udi_error_e install_breakpoint(udi_process *proc, udi_address addr);
udi_error_e remove_breakpoint(udi_process *proc, udi_address addr);
udi_error_e delete_breakpoint(udi_process *proc, udi_address addr);

// Memory access interface //
udi_error_e mem_access(udi_process *proc, int write, void *value, 
        udi_length size, udi_address addr);

// Event handling interface //

/**
 * Encapsulates an event in the debuggee
 */
typedef struct udi_event_struct {
    udi_event_type_e event_type;
    udi_process *proc;
    udi_thread *thr;
    void *event_data;
    struct udi_event_struct *next_event;
} udi_event;

/**
 * When udi_event.event_type == UDI_EVENT_ERROR,
 * typeof(udi_event.event_data) == udi_event_error
 */
typedef struct udi_event_error_struct {
    char *errstr;
} udi_event_error;

/**
 * When udi_event.event_type == UDI_EVENT_PROCESS_EXIT
 * typeof(udi_event.event_data) == udi_event_process_exit
 */
typedef struct udi_event_process_exit_struct {
    int exit_code;
} udi_event_process_exit;

/**
 * When udi_event.event_type == UDI_EVENT_BREAKPOINT
 * typeof(udi_event.event_data) == udi_event_breakpoint
 */
typedef struct udi_event_breakpoint_struct {
    udi_address breakpoint_addr;
} udi_event_breakpoint;

udi_event *wait_for_events(udi_process *procs[], int num_procs);
const char *get_event_type_str(udi_event_type event_type);
void free_event_list(udi_event *event_list);

#ifdef __cplusplus
} // "C"
#endif

#endif
