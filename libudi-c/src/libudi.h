/*
 * Copyright (c) 2011-2017, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _LIBUDI_H
#define _LIBUDI_H 1

#include <stdint.h>

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
    UDI_ERROR_NOMEM, /// memory could not be allocated
    UDI_ERROR_NONE
} udi_error_e;

/**
 * Error structure
 */
typedef struct udi_error_struct {
    udi_error_e code;

    /**
     * A heap allocated error string. It will be null if memory could not be allocated for the error message.
     * Callers of UDI functions are expected to free this value.
     */
    const char *const msg;
} udi_error;

/**
 * architecture of debuggee
 */
typedef enum {
    UDI_ARCH_X86 = 0,
    UDI_ARCH_X86_64
} udi_arch_e;

/**
 * The version of the protocol
 */
typedef enum {
    UDI_PROTOCOL_VERSION_1 = 1
} udi_version_e;

/**
 * The running state for a thread
 */
typedef enum {
    UDI_TS_RUNNING = 0,
    UDI_TS_SUSPENDED,
} udi_thread_state_e;

/**
 * Registers
 */
typedef enum {
    // X86 registers
    UDI_X86_MIN = 0,
    UDI_X86_GS,
    UDI_X86_FS,
    UDI_X86_ES,
    UDI_X86_DS,
    UDI_X86_EDI,
    UDI_X86_ESI,
    UDI_X86_EBP,
    UDI_X86_ESP,
    UDI_X86_EBX,
    UDI_X86_EDX,
    UDI_X86_ECX,
    UDI_X86_EAX,
    UDI_X86_CS,
    UDI_X86_SS,
    UDI_X86_EIP,
    UDI_X86_FLAGS,
    UDI_X86_ST0,
    UDI_X86_ST1,
    UDI_X86_ST2,
    UDI_X86_ST3,
    UDI_X86_ST4,
    UDI_X86_ST5,
    UDI_X86_ST6,
    UDI_X86_ST7,
    UDI_X86_MAX,

    //UDI_X86_64 registers
    UDI_X86_64_MIN,
    UDI_X86_64_R8,
    UDI_X86_64_R9,
    UDI_X86_64_R10,
    UDI_X86_64_R11,
    UDI_X86_64_R12,
    UDI_X86_64_R13,
    UDI_X86_64_R14,
    UDI_X86_64_R15,
    UDI_X86_64_RDI,
    UDI_X86_64_RSI,
    UDI_X86_64_RBP,
    UDI_X86_64_RBX,
    UDI_X86_64_RDX,
    UDI_X86_64_RAX,
    UDI_X86_64_RCX,
    UDI_X86_64_RSP,
    UDI_X86_64_RIP,
    UDI_X86_64_CSGSFS,
    UDI_X86_64_FLAGS,
    UDI_X86_64_ST0,
    UDI_X86_64_ST1,
    UDI_X86_64_ST2,
    UDI_X86_64_ST3,
    UDI_X86_64_ST4,
    UDI_X86_64_ST5,
    UDI_X86_64_ST6,
    UDI_X86_64_ST7,
    UDI_X86_64_XMM0,
    UDI_X86_64_XMM1,
    UDI_X86_64_XMM2,
    UDI_X86_64_XMM3,
    UDI_X86_64_XMM4,
    UDI_X86_64_XMM5,
    UDI_X86_64_XMM6,
    UDI_X86_64_XMM7,
    UDI_X86_64_XMM8,
    UDI_X86_64_XMM9,
    UDI_X86_64_XMM10,
    UDI_X86_64_XMM11,
    UDI_X86_64_XMM12,
    UDI_X86_64_XMM13,
    UDI_X86_64_XMM14,
    UDI_X86_64_XMM15,
    UDI_X86_64_MAX
} udi_register_e;

// Process management //

/**
 * Configuration structure for a process
 */
typedef struct udi_proc_config_struct {
    const char *root_dir;
    const char *rt_lib_path;
} udi_proc_config;

/*
 * Create UDI-controlled process
 *
 * @param executable   the full path to the executable
 * @param argv         the arguments
 * @param envp         the environment, if NULL, the newly created process will
 *                     inherit the environment for this process
 * @param config       the configuration for creating the process
 * @param process      populated on success
 *
 * @return the result of the operation
 *
 * @return the result of the operation
 * @see execve on a UNIX system
 */
udi_error create_process(const char *executable,
                         const char * const *argv,
                         const char * const *envp,
                         const udi_proc_config *config,
                         udi_process * const* process);

/**
 * Tells the library that resources allocated for the process can be released
 *
 * @param proc          the process handle
 */
void free_process(udi_process *proc);

/**
 * Continue a stopped UDI process
 *
 * @param proc          the process handle
 *
 * @return the result of the operation
 */
udi_error continue_process(udi_process *proc);

/**
 * Refreshes the state of the specified process
 *
 * @param proc          the process handle
 *
 * @return the result of the operation
 */
udi_error refresh_state(udi_process *proc);

// Process properties //

/**
 * Sets the user data stored with the internal process structure
 *
 * @param proc          the process handle
 * @param user_data     the user data to associated with the process handle
 *
 * @return the result of the operation
 */
udi_error set_user_data(udi_process *proc, void *user_data);

/**
 * Gets the user data stored with the internal process structure
 *
 * @param proc          the process handle
 * @param output     the output user data
 *
 * @return the result of the operation
 */
udi_error *get_user_data(udi_process *proc, void **output);

/**
 * Gets the process identifier for the specified process
 *
 * @param proc          the process handle
 * @param output        the output pid
 *
 * @return the result of the operation
 */
udi_error get_proc_pid(udi_process *proc, uint32_t *output);

/**
 * Gets the architecture for the specified process
 *
 * @param proc          the process handle
 * @param output        the output architecture
 *
 * @return the result of the operation
 */
udi_error get_proc_architecture(udi_process *proc, udi_arch_e *output);

/**
 * Gets whether the specified process is multithread capable
 *
 * @param proc          the process handle
 * @param output        the output. non-zero if the process is multithread capable
 *
 * @return the result of the operation
 */
udi_error get_multithread_capable(udi_process *proc, int *output);

/*
 * Gets the initial thread in the specified process
 *
 * @param proc the process handle
 * @param output the output
 *
 * @return the result of the operation
 */
udi_error get_initial_thread(udi_process *proc, udi_thread **output);

/**
 * @param proc the process handle
 * @param output non-zero if the process has been continued, but events haven't been received yet
 *
 * @return the result of the operation
 */
udi_error is_running(udi_process *proc, int *output);

/**
 * @param proc the process handle
 * @param output non-zero if the process is terminated and can no longer be interacted with
 *
 * @return the result of the operation
 */
udi_error is_terminated(udi_process *proc, int *output);

// Thread properties //

/**
 * Sets the user data stored with the internal thread structure
 *
 * @param thread       the thread handle
 * @param user_data    the user data
 *
 * @return the result of the operation
 */
udi_error set_thread_user_data(udi_thread *thr, void *user_data);

/**
 * Gets the user data stored with the internal thread structure
 *
 * @param thread       the thread handle
 * @param output       the output user data
 *
 * @return the result of the operation
 */
udi_error *get_thread_user_data(udi_thread *thr, void **output);

/**
 * Gets the thread id for the specified thread
 *
 * @param thr the thread handle
 * @param output the output tid
 *
 * @return the result of the operation
 */
udi_error get_tid(udi_thread *thr, uint64_t *output);

/**
 * Gets the state for the specified thread
 *
 * @param thr the thread handle
 * @param output the state
 *
 * @return the result of the operation
 */
udi_error get_state(udi_thread *thr, udi_thread_state_e *output);

/**
 * Gets the next thread
 * 
 * @param thr the thread
 * @param output the next thread or NULL if no more threads
 *
 * @return the result of the operation
 */
udi_error get_next_thread(udi_process *proc,
                          udi_thread *thr,
                          udi_thread **output);

// Thread control //

/**
 * Resumes the specified thread.
 *
 * @return the result of the operation
 */
udi_error resume_thread(udi_thread *thr);

/**
 * Suspend the specified thread.
 *
 * @return the result of the operation. it is an error to suspend all the threads in a process
 */
udi_error suspend_thread(udi_thread *thr);

/**
 * Sets the single step setting for a specific thread
 *
 * @param thr the thread
 * @param enable the new setting
 *
 * @return the result of the operation
 */
udi_error set_single_step(udi_thread *thr, int enable);

/**
 * @param thr the thread
 * @param output the output setting
 *
 * @return the result of the operation
 */
udi_error get_single_step(udi_thread *thr, int *output);

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
udi_error create_breakpoint(udi_process *proc, uint64_t addr);

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
udi_error install_breakpoint(udi_process *proc, uint64_t addr);

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
udi_error remove_breakpoint(udi_process *proc, uint64_t addr);

/**
 *
 * Delete a previously created breakpoint for the specified process
 *
 * @param proc          the process handle
 * @param addr          the address of the breakpoint
 *
 * @return the result of the operation
 */
udi_error delete_breakpoint(udi_process *proc, uint64_t addr);

// Memory access interface //

/**
 * Read memory from a process
 *
 * @param proc          the process handle
 * @param dst           the destination for the read memory
 * @param size          the size of the data block to read
 * @param addr          the address of the data block to read
 */
udi_error read_mem(udi_process *proc,
                   uint8_t *dst,
                   uint32_t size,
                   uint64_t addr);

/**
 * Write memory from a process
 *
 * @param proc          the process handle
 * @param src           the source of the data to write
 * @param size          the size of the data block to write
 * @param addr          the address of the data block to write
 */
udi_error write_mem(udi_process *proc,
                    const uint8_t *src,
                    uint32_t size,
                    uint64_t addr);

/**
 * Reads a register for the specified thread
 *
 * @param thr the thread
 * @param reg the register
 * @param value the output value
 *
 * @return the result of the operation
 */
udi_error read_register(udi_thread *thr, udi_register_e reg, uint64_t *value);

/**
 * Write a register for the specified thread
 *
 * @param thr the thread
 * @param reg the register
 * @param value the output value
 *
 * @return the result of the operation
 */
udi_error write_register(udi_thread *thr, udi_register_e reg, uint64_t value);

/**
 * Gets the PC for the specified thread
 *
 * @param thr the thread
 * @param pc the output parameter for the pc
 *
 * @return the result of the operation
 */
udi_error get_pc(udi_thread *thr, uint64_t *pc);

/**
 * Gets the next instruction to be executed by the specified thread
 *
 * @param thr the thread
 * @param instr the output parameter for the instruction address
 *
 * @return the result of the operation
 */
udi_error get_next_instruction(udi_thread *thr, uint64_t *instr);

// Event handling interface //

/*
 * Event types
 */
typedef enum
{
    UDI_EVENT_UNKNOWN = 0,
    UDI_EVENT_ERROR,
    UDI_EVENT_SIGNAL,
    UDI_EVENT_BREAKPOINT,
    UDI_EVENT_THREAD_CREATE,
    UDI_EVENT_THREAD_DEATH,
    UDI_EVENT_PROCESS_EXIT,
    UDI_EVENT_PROCESS_FORK,
    UDI_EVENT_PROCESS_EXEC,
    UDI_EVENT_SINGLE_STEP,
    UDI_EVENT_PROCESS_CLEANUP
} udi_event_type_e;

/**
 * Encapsulates an event in the debuggee
 */
typedef struct udi_event_struct {
    udi_event_type_e event_type;
    udi_process *process;
    udi_thread *thr;
    void *event_data;
    struct udi_event_struct *next_event;
} udi_event;

/**
 * When udi_event.event_type == UDI_EVENT_ERROR,
 * typeof(udi_event.event_data) == udi_event_error
 */
typedef struct udi_event_error_struct {
    const char *errstr;
} udi_event_error;

/**
 * When udi_event.event_type == UDI_EVENT_PROCESS_EXIT
 * typeof(udi_event.event_data) == udi_event_process_exit
 */
typedef struct udi_event_process_exit_struct {
    int32_t exit_code;
} udi_event_process_exit;

/**
 * When udi_event.event_type == UDI_EVENT_PROCESS_FORK
 * typeof(udi_event.event_data) == udi_event_process_fork
 */
typedef struct udi_event_process_fork_struct {
    uint32_t pid;
} udi_event_process_fork;

/**
 * When udi_event.event_type == UDI_EVENT_PROCESS_EXEC
 * typeof(udi_event.event_data) == udi_event_process_exec
 */
typedef struct udi_event_process_exec_struct {
    const char *path;
    const char * const *argv;
    const char * const *envp;
} udi_event_process_exec;

/**
 * When udi_event.event_type == UDI_EVENT_BREAKPOINT
 * typeof(udi_event.event_data) == udi_event_breakpoint
 */
typedef struct udi_event_breakpoint_struct {
    uint64_t breakpoint_addr;
} udi_event_breakpoint;

/**
 * When udi_event.event_type == UDI_EVENT_THREAD_CREATE
 * typeof(udi_event.event_data) == udi_event_thread_create
 */
typedef struct udi_event_thread_create_struct {
    udi_thread *new_thr;
} udi_event_thread_create;

/**
 * When udi_event.event_type == UDI_EVENT_SIGNAL
 * typeof(udi_event.event_data) == udi_event_signal
 */
typedef struct udi_event_signal_struct {
    uint64_t addr;
    uint32_t sig;
} udi_event_signal;

/**
 * Wait for events to occur in the specified processes.
 *
 * @param procs         the processes
 * @param num_procs     the number of processes
 * @param events        the output events or NULL if no events
 *
 * @return the result of the operation
 */
udi_error wait_for_events(udi_process *procs[], int num_procs, udi_event **events);

/**
 * @return a string representation of the specified event type
 */
const char *get_event_type_str(udi_event_type_e event_type);

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
