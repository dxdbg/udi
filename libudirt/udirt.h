/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// UDI debuggee implementation common between all platforms

#ifndef _UDI_RT_H
#define _UDI_RT_H 1

#include "udi.h"
#include "udi-common.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// global variables
extern char *UDI_ROOT_DIR;
extern int udi_enabled;
extern int udi_debug_on;

// General platform-specific functions
void udi_abort(const char *file, unsigned int line);

// UDI RT internal malloc
void udi_set_max_mem_size(unsigned long max_size);
void udi_free(void *ptr);
void *udi_malloc(size_t length);
void dump_heap();

unsigned char *map_mem(size_t length);
int unmap_mem(void *addr, size_t length);

// opaque (at this level) thread handle
typedef struct thread_struct thread;

// request handling
udi_version_e get_protocol_version();
int write_response(udi_response *response);
int write_response_to_request(udi_response *response);
int write_response_to_thr_request(thread *thr, udi_response *response);
int write_event(udi_event_internal *event);
udi_request *read_request(thread **thr);
void free_request(udi_request *request);

extern const int REQ_SUCCESS;
extern const int REQ_ERROR;
extern const int REQ_FAILURE;

// reading and writing debugee memory
void *get_mem_access_addr();
size_t get_mem_access_size();

unsigned long abort_mem_access();
int is_performing_mem_access();

void *pre_mem_access_hook();
int post_mem_access_hook(void *hook_arg);

int read_memory(void *dest, const void *src, size_t num_bytes, udi_errmsg *errmsg);
int write_memory(void *dest, const void *src, size_t num_bytes, udi_errmsg *errmsg);

const char *get_mem_errstr();

// disassembly interface
unsigned long get_ctf_successor(unsigned long pc, udi_errmsg *errmsg, void *context);

// register interface
int get_register(udi_arch_e arch, udi_register_e reg, udi_errmsg *errmsg, udi_address *value, 
        const void *context);
int set_register(udi_arch_e arch, udi_register_e reg, udi_errmsg *errmsg, udi_address value,
        void *context);
int is_gp_register(udi_arch_e arch, udi_register_e reg);
int is_fp_register(udi_arch_e arch, udi_register_e reg);

// breakpoint handling
typedef struct breakpoint_struct {
    unsigned char saved_bytes[8];
    udi_address address;
    unsigned char in_memory;
    thread *thread; // NULL if the breakpoint is set for all threads
    struct breakpoint_struct *next_breakpoint;
} breakpoint;

breakpoint *create_breakpoint(udi_address breakpoint_addr);

int install_breakpoint(breakpoint *bp, udi_errmsg *errmsg);
int remove_breakpoint(breakpoint *bp, udi_errmsg *errmsg);
int remove_breakpoint_for_continue(breakpoint *bp, udi_errmsg *errmsg);
int delete_breakpoint(breakpoint *bp, udi_errmsg *errmsg);
breakpoint *find_breakpoint(udi_address breakpoint_addr);

// architecture specific breakpoint handling
int write_breakpoint_instruction(breakpoint *bp, udi_errmsg *errmsg);
int write_saved_bytes(breakpoint *bp, udi_errmsg *errmsg);
udi_arch_e get_architecture();

// error logging
#define udi_printf(format, ...) \
    do {\
        if( udi_debug_on ) {\
            fprintf(stderr, "%s[%d]: " format, __FILE__, __LINE__,\
                    ## __VA_ARGS__);\
        }\
    }while(0)

#ifdef __cplusplus
} // extern C
#endif

#endif
