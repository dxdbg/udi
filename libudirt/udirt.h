/*
 * Copyright (c) 2011, Dan McNulty
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
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

#ifndef _UDI_RT_H
#define _UDI_RT_H 1

#include "udi.h"
#include "udi-common.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// platform independent settings */
extern char *UDI_ROOT_DIR;
extern int udi_enabled;
extern int udi_debug_on;
extern int udi_in_sig_handler;

// platform specific functions

void udi_free(void *ptr);
void *udi_malloc(size_t length);

// platform independent functions

int write_response(udi_response *response);
int write_response_to_request(udi_response *response);
int write_event(udi_event_internal *event);
udi_request *read_request();
void free_request(udi_request *request);

// request handling

/** Request processed successfully */
extern const int REQ_SUCCESS;

/** Failure to process request due to environment/OS error, unrecoverable */
extern const int REQ_ERROR;

/** Failure to process request due to invalid arguments */
extern const int REQ_FAILURE;

// reading and writing debugee memory

extern void *mem_access_addr;
extern size_t mem_access_size;

extern int abort_mem_access;
extern int performing_mem_access;

const char *get_mem_errstr();

int read_memory(void *dest, const void *src, size_t num_bytes,
        char *errmsg, unsigned int errmsg_size);

int write_memory(void *dest, const void *src, size_t num_bytes,
        char *errmsg, unsigned int errmsg_size);

int failed_mem_access_response(udi_request_type request_type, char *errmsg,
        unsigned int errmsg_size);

// breakpoint handling

typedef struct breakpoint_struct {
    unsigned char saved_bytes[8];
    int instruction_length;
    udi_address address;
    int in_memory;
    struct breakpoint_struct *next_breakpoint;
} breakpoint;

breakpoint *create_breakpoint(udi_address breakpoint_addr, int instruction_length);

int install_breakpoint(breakpoint *bp, char *errmsg, 
        unsigned int errmsg_size);

int remove_breakpoint(breakpoint *bp, char *errmsg,
        unsigned int errmsg_size);

int delete_breakpoint(breakpoint *bp, char *errmsg,
        unsigned int errmsg_size);

breakpoint *find_breakpoint(udi_address breakpoint_addr);

// architecture specific breakpoint handling
int write_breakpoint_instruction(breakpoint *bp, char *errmsg,
        unsigned int errmsg_size);

int write_saved_bytes(breakpoint *bp, char *errmsg,
        unsigned int errmsg_size);

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
