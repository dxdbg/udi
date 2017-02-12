/*
 * Copyright (c) 2011-2017, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// UDI debuggee implementation common between all platforms

#ifndef _UDI_RT_H
#define _UDI_RT_H 1

#include <stdio.h>
#include <stdint.h>

#include "udi.h"
#include "udirt-platform.h"

#ifdef __cplusplus
extern "C" {
#endif

// Macros //
#define member_sizeof(s,m) ( sizeof( ((s *)0)->m ) )

// global variables //

// platform independent variables and constants
extern const char * const UDI_ROOT_DIR_ENV;
extern const char * const REQUEST_FILE_NAME;
extern const char * const RESPONSE_FILE_NAME;
extern const char * const EVENTS_FILE_NAME;
extern const char * const UDI_DEBUG_ENV;
extern const uint64_t UDI_SINGLE_THREAD_ID;

extern int udi_enabled;
extern int udi_debug_on;

// platform specific variables and constants
extern const char * const DEFAULT_UDI_ROOT_DIR;
extern const char * const UDI_DS;
extern const unsigned int DS_LEN;

extern char *UDI_ROOT_DIR;

// General platform-specific functions
void udi_abort(const char *file, unsigned int line);
int read_from(udirt_fd fd, uint8_t *dst, size_t length);
int write_to(udirt_fd fd, const uint8_t *src, size_t length);

// UDI RT internal malloc
void udi_free(void *ptr);
void *udi_malloc(size_t length);
void *udi_calloc(size_t count, size_t size);
void *udi_realloc(void *ptr, size_t length);

// helper functions
const char *request_type_str(udi_request_type_e req_type); 
const char *event_type_str(udi_event_type_e event_type);
const char *arch_str(udi_arch_e arch);
const char *register_str(udi_register_e reg);

// error handling

/** Operation successful */
extern const int RESULT_SUCCESS;

/** Unrecoverable failure caused by environment/OS error */
extern const int RESULT_ERROR;

/** Failure to process request due to invalid arguments */
extern const int RESULT_FAILURE;

#define ERRMSG_SIZE 4096
typedef struct {
    char msg[ERRMSG_SIZE];
    unsigned int size;
} udi_errmsg;

// threads
typedef struct thread_struct thread;
typedef struct breakpoint_struct breakpoint;

uint64_t get_user_thread_id();
int get_num_threads();
int get_multithread_capable();
int get_multithreaded();
thread *get_thread_list();
thread *get_next_thread(thread *thr);
thread *get_current_thread();
int is_thread_dead(thread *thr);
udi_thread_state_e get_thread_state(thread *thr);
void set_thread_state(thread *thr, udi_thread_state_e state);
uint64_t get_thread_id(thread *thr);
int is_thread_context_valid(thread *thr);
void *get_thread_context(thread *thr);
int is_single_step(thread *thr);
void set_single_step(thread *thr, int single_step);
breakpoint *get_single_step_breakpoint(thread *thr);
void set_single_step_breakpoint(thread *thr, breakpoint *bp);

int thread_death_handshake(thread *thr, udi_errmsg *errmsg);

// request handling
udi_version_e get_protocol_version();

void init_req_handling();
int handle_process_request(udirt_fd req_fd,
                           udirt_fd resp_fd,
                           udi_request_type_e *type,
                           udi_errmsg *errmsg);
int handle_thread_request(udirt_fd req_fd,
                          udirt_fd resp_fd,
                          thread *thr,
                          udi_request_type_e *type,
                          udi_errmsg *errmsg);

typedef int (*response_fd_callback)(void *ctx, udirt_fd *resp_fd, udi_errmsg *errmsg);
int perform_init_handshake(udirt_fd req_fd,
                           response_fd_callback callback,
                           void *ctx,
                           uint64_t tid,
                           udi_errmsg *errmsg);

// reading and writing debuggee memory
void *get_mem_access_addr();
size_t get_mem_access_size();

unsigned long abort_mem_access();
int is_performing_mem_access();

void *pre_mem_access_hook();
int post_mem_access_hook(void *hook_arg);

int read_memory(void *dest, const void *src, size_t num_bytes, udi_errmsg *errmsg);
int write_memory(void *dest, const void *src, size_t num_bytes, udi_errmsg *errmsg);

const char *get_mem_errstr();

// disassembly interface //

/**
 * Gets the control flow successor for the instruction at the specified pc
 *
 * @param pc the program counter
 * @param errmsg the error message populated on failure
 * @param context the context from which registers can be retrieved
 *
 * @return the address of the control flow successor or 0 on error
 */
uint64_t get_ctf_successor(uint64_t pc, udi_errmsg *errmsg, const void *context);

// register interface //

int get_register(udi_arch_e arch,
                 udi_register_e reg,
                 udi_errmsg *errmsg,
                 uint64_t *value,
                 const void *context);
int set_register(udi_arch_e arch,
                 udi_register_e reg,
                 udi_errmsg *errmsg,
                 uint64_t value,
                 void *context);
int is_gp_register(udi_arch_e arch, udi_register_e reg);
int is_fp_register(udi_arch_e arch, udi_register_e reg);

/**
 * Given the context, gets the pc
 *
 * @param context the context containing the current PC value
 *
 * @return the PC contained in the context
 */
uint64_t get_pc(const void *context);

/**
 * Given the context, rewinds the PC to account for a hit breakpoint
 *
 * @param context the context containing the current PC value
 */
void rewind_pc(void *context);

// breakpoint handling
struct breakpoint_struct {
    unsigned char saved_bytes[8];
    uint64_t address;
    unsigned char in_memory;
    thread *thread; // NULL if the breakpoint is set for all threads
    struct breakpoint_struct *next_breakpoint;
};

breakpoint *create_breakpoint(uint64_t breakpoint_addr);

int install_breakpoint(breakpoint *bp, udi_errmsg *errmsg);
int remove_breakpoint(breakpoint *bp, udi_errmsg *errmsg);
int remove_breakpoint_for_continue(breakpoint *bp, udi_errmsg *errmsg);
int delete_breakpoint(breakpoint *bp, udi_errmsg *errmsg);
breakpoint *find_breakpoint(uint64_t breakpoint_addr);

// architecture specific breakpoint handling
int write_breakpoint_instruction(breakpoint *bp, udi_errmsg *errmsg);
int write_saved_bytes(breakpoint *bp, udi_errmsg *errmsg);
udi_arch_e get_architecture();

// continue handling //

/** The breakpoint used to single step from a user breakpoint */
extern breakpoint *continue_bp;

/**
 * A hook ran after the continue response has been sent
 *
 * @param sig_val the value of the signal to continue the process with
 */
void post_continue_hook(uint32_t sig_val);

// event reporting

extern udirt_fd events_handle;

/**
 * Handles the breakpoint event that occurred at the specified breakpoint
 *
 * @param thr the thread that hit the breakpoint
 * @param bp the breakpoint that triggered the event
 * @param context the context passed to the signal handler
 * @param wait_for_request populated on success
 * @param errmsg the error message populated on error
 *
 * @return the result of decoding the event
 */
int decode_breakpoint(thread *thr,
                      breakpoint *bp,
                      void *context,
                      int *wait_for_request,
                      udi_errmsg *errmsg);


/**
 * @param bp the breakpoint
 *
 * @return non-zero if the specified breakpoint is a event breakpoint; zero otherwise
 */
int is_event_breakpoint(breakpoint *bp);

/**
 * Handles the specified event breakpoint
 *
 * @param bp the breakpoint
 * @param context the context
 * @param errmsg the error message populated on error
 */
int handle_event_breakpoint(breakpoint *bp,
                            const void *context,
                            udi_errmsg *errmsg);

int handle_thread_create_event(uint64_t creator_tid,
                               uint64_t tid,
                               udi_errmsg *errmsg);

int handle_thread_death_event(uint64_t tid,
                              udi_errmsg *errmsg);

int handle_unknown_event(uint64_t tid, udi_errmsg *errmsg);

int handle_error_event(uint64_t tid, udi_errmsg *errmsg);

int handle_exit_event(uint64_t tid, int32_t status, udi_errmsg *errmsg);

int handle_fork_event(uint64_t tid, uint32_t pid, udi_errmsg *errmsg);

// logging
#define udi_printf(format, ...) \
    do {\
        if( udi_debug_on ) {\
            fprintf(stderr, "%s[%d]: " format, __FILE__, __LINE__,\
                    ## __VA_ARGS__);\
        }\
    }while(0)

#define udi_printf_noprefix(format, ...) \
    do {\
        if ( udi_debug_on ) {\
            fprintf(stderr, format, ## __VA_ARGS__);\
        }\
    }while(0)

#ifdef __cplusplus
} // extern C
#endif

#endif
