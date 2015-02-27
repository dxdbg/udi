/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _LIBUDI_H
#define _LIBUDI_H 1

#include "udi.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque process handle */
typedef struct udi_process_struct udi_process;

/** Opaque thread handle */
typedef struct udi_thread_struct udi_thread;

/**
 * library error codes
 */
typedef enum {
    UDI_ERROR_LIBRARY, /// there was an internal library error
    UDI_ERROR_REQUEST, /// the request was invalid
    UDI_ERROR_NONE
} udi_error_e;

// Process management //

/**
 * Configuration structure for a process
 */
typedef struct udi_proc_config_struct {
    const char *root_dir;
} udi_proc_config;

/*
 * Create UDI-controlled process
 * 
 * @param executable   the full path to the executable
 * @param argv         the arguments
 * @param envp         the environment, if NULL, the newly created process will
 *                     inherit the environment for this process
 * @param config       the configuration for creating the process
 * @param error_code   on error, populated with the error code
 * @param errmsg       on error, populated with the error message (allocated by malloc)
 *
 * @return a handle to the created process
 *
 * @see execve on a UNIX system
 */
udi_process *create_process(const char *executable, char * const argv[],
        char * const envp[], const udi_proc_config *config,
        udi_error_e *error_code, char **errmsg);

/**
 * Tells the library that resources allocated for the process can be released
 *
 * @param proc          the process handle
 *
 * @return UDI_ERROR_NONE if the resources are released successfully
 */
udi_error_e free_process(udi_process *proc);

/**
 * Continue a stopped UDI process
 *
 * @param proc          the process handle
 *
 * @return the result of the operation
 */
udi_error_e continue_process(udi_process *proc);

/**
 * Refreshes the state of the specified process
 *
 * @param proc          the process handle
 *
 * @return the result of the operation
 */
udi_error_e refresh_state(udi_process *proc);

// Process properties //

/**
 * Sets the user data stored with the internal process structure
 *
 * @param proc          the process handle
 * @param user_data     the user data to associated with the process handle
 */
void set_user_data(udi_process *proc, void *user_data);

/**
 * Gets the user data stored with the internal process structure
 *
 * @param proc          the process handle
 *
 * @return the user data
 */
void *get_user_data(udi_process *proc);

/**
 * Gets the process identifier for the specified process
 *
 * @param proc          the process handle
 *
 * @return the pid for the process
 */
int get_proc_pid(udi_process *proc);

/**
 * Gets the architecture for the specified process
 *
 * @param proc          the process handle
 *
 * @return the architecture for the process
 */
udi_arch_e get_proc_architecture(udi_process *proc);

/**
 * Gets whether the specified process is multithread capable
 *
 * @param proc          the process handle
 *
 * @return non-zero if the process is multithread capable
 */
int get_multithread_capable(udi_process *proc);

/*
 * Gets the initial thread in the specified process
 *
 * @param proc the process handle
 *
 * @return the thread handle or NULL if no initial thread could be found
 */
udi_thread *get_initial_thread(udi_process *proc);

/**
 * @param proc the process handle
 *
 * @return non-zero if the process has been continued, but events haven't been received yet
 */
int is_running(udi_process *proc);

/**
 * @param proc the process handle
 *
 * @return non-zero if the process is terminated and can no longer be interacted with
 */
int is_terminated(udi_process *proc);

// Thread properties //

/**
 * Sets the user data stored with the internal thread structure
 *
 * @param thread       the thread handle
 * @param user_data    the user data
 */
void set_thread_user_data(udi_thread *thr, void *user_data);

/**
 * Gets the user data stored with the internal thread structure
 *
 * @param thread       the thread handle
 *
 * @return the user data
 */
void *get_thread_user_data(udi_thread *thr);

/**
 * Gets the thread id for the specified thread
 *
 * @param thr the thread handle
 *
 * @return the thread id for the thread
 */
uint64_t get_tid(udi_thread *thr);

/**
 * Gets the process for the specified thread
 *
 * @param thr the thread handle
 *
 * @return the process handle
 */
udi_process *get_process(udi_thread *thr);

/**
 * Gets the state for the specified thread
 *
 * @param thr thre thread handle
 *
 * @return the thread handle
 */
udi_thread_state_e get_state(udi_thread *thr);

/**
 * Gets the next thread
 * 
 * @param thr the thread
 *
 * @return the thread or NULL if no more threads
 */
udi_thread *get_next_thread(udi_thread *thr);

// Thread control //

/**
 * Resumes the specified thread.
 *
 * @return the result of the operation
 */
udi_error_e resume_thread(udi_thread *thr);

/**
 * Suspend the specified thread.
 *
 * @return the result of the operation. it is an error to suspend all the threads in a process
 */
udi_error_e suspend_thread(udi_thread *thr);

/**
 * Sets the single step setting for a specific thread
 *
 * @param thr the thread
 * @param enable the new setting
 *
 * @return the result of the operation
 */
udi_error_e set_single_step(udi_thread *thr, int enable);

/**
 * @param thr the thread
 *
 * @return the current single step setting for the specified thread
 */
int get_single_step(udi_thread *thr);

// Breakpoint interface //

/**
 * Creates a breakpoint in the specified process at the specified
 * virtual address
 *
 * @param proc          the process handle
 * @param addr          the address to place the breakpoint
 * 
 * @return the result of the operation
 */
udi_error_e create_breakpoint(udi_process *proc, udi_address addr);

/**
 *
 * Install a previously created breakpoint into the specified process'
 * memory
 *
 * @param proc          the process handle
 * @param addr          the address of the breakpoint
 *
 * @return the result of the operation
 */
udi_error_e install_breakpoint(udi_process *proc, udi_address addr);

/**
 *
 * Remove a previously installed breakpoint from the specified process'
 * memory
 *
 * @param proc          the process handle
 * @param addr          the address of the breakpoint
 *
 * @return the result of the operation
 */
udi_error_e remove_breakpoint(udi_process *proc, udi_address addr);

/**
 *
 * Delete a previously created breakpoint for the specified process
 *
 * @param proc          the process handle
 * @param addr          the address of the breakpoint
 *
 * @return the result of the operation
 */
udi_error_e delete_breakpoint(udi_process *proc, udi_address addr);

// Memory access interface //

/**
 * Access memory in a process
 *
 * @param proc          the process handle
 * @param write         if non-zero, write to specified address
 * @param value         pointer to the value to read/write
 * @param size          the size of the data block pointed to by value
 * @param addr          the location in memory to read/write
 *
 * @return the result of the operation
 */
udi_error_e mem_access(udi_process *proc, int write, void *value, 
        udi_length size, udi_address addr);

// Register access interface //

/**
 * Access a register in the specified thread
 *
 * @param thr the thread
 * @param write 1 if the value should written into the register, 0 read
 * @param reg the register to access
 * @param value the destination of the register value on read; the value to write otherwise
 *
 * @return the result of the operation
 */
udi_error_e register_access(udi_thread *thr, int write, udi_register_e reg, udi_address *value);

/**
 * Gets the PC for the specified thread
 *
 * @param thr the thread
 * @param pc the output parameter for the pc
 *
 * @return the result of the operation
 */
udi_error_e get_pc(udi_thread *thr, udi_address *pc);

/**
 * Gets the next instruction to be executed by the specified thread
 *
 * @param thr the thread
 * @param instr the output parameter for the instruction address
 *
 * @return the result of the operation
 */
udi_error_e get_next_instruction(udi_thread *thr, udi_address *instr);

// Error handling //

/**
 * Get a message describing the passed error code
 *
 * @param proc the process
 */
const char *get_last_error_message(udi_process *proc);

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

/**
 * When udi_event.event_type == UDI_EVENT_THREAD_CREATE
 * typeof(udi_event.event_data) == udi_event_thread_create
 */
typedef struct udi_event_thread_create_struct {
    udi_thread *new_thr;
} udi_event_thread_create;

/**
 * Wait for events to occur in the specified processes.
 *
 * @param procs         the processes
 * @param num_procs     the number of processes
 *
 * @return a list of events that occurred in the processes, NULL on failure
 */
udi_event *wait_for_events(udi_process *procs[], int num_procs);

/**
 * @return a string representation of the specified event type
 */
const char *get_event_type_str(udi_event_type event_type);

/**
 * Frees a event list returned by wait_for_events
 *
 * @param event_list the event list to free
 */
void free_event_list(udi_event *event_list);

#ifdef __cplusplus
} // "C"
#endif

#endif
