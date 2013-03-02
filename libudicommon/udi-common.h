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

/* shared functionality between UDI debugger and debuggee */

#ifndef _UDI_COMMON_H
#define _UDI_COMMON_H 1

#include "udi.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Communication settings
 *
 * These variables facilitate communication between the debugger and debuggee
 */

/* platform independent variables */
extern const char *UDI_ROOT_DIR_ENV;
extern const char *REQUEST_FILE_NAME;
extern const char *RESPONSE_FILE_NAME;
extern const char *EVENTS_FILE_NAME;
extern const char *UDI_DEBUG_ENV;

/* platform specific variables */
extern const char *DEFAULT_UDI_ROOT_DIR;
extern const char *UDI_DS;
extern const unsigned int DS_LEN;

/* useful macro */
#define member_sizeof(s,m) ( sizeof( ((s *)0)->m ) )

/*
 * data structures
 */
typedef struct {
    udi_response_type response_type;
    udi_request_type request_type;
    udi_length length;
    void *packed_data;
} udi_response;

typedef struct {
    udi_request_type request_type;
    udi_length length;
    void *packed_data;
} udi_request;

typedef struct {
    udi_event_type event_type;
    uint64_t thread_id;
    udi_length length;
    void *packed_data;
} udi_event_internal;

#define ERRMSG_SIZE 4096
typedef struct {
    char msg[ERRMSG_SIZE];
    unsigned int size;
} udi_errmsg;

/* helper functions */

const char *request_type_str(udi_request_type req_type); 
const char *event_type_str(udi_event_type event_type);

/* self-contained data structure serialization */
typedef void *(*malloc_type)(size_t);
extern malloc_type data_allocator;
void udi_set_malloc(malloc_type allocator);

void *udi_pack_data(udi_length length, ...);
int udi_unpack_data(void *data, udi_length length, ...);

/* network order packing */
udi_address udi_address_hton(udi_address value);
udi_address udi_address_ntoh(udi_address value);
udi_length udi_length_hton(udi_length value);
udi_length udi_length_ntoh(udi_length value);
udi_request_type udi_request_type_hton(udi_request_type value);
udi_request_type udi_request_type_ntoh(udi_request_type value);
udi_response_type udi_response_type_hton(udi_response_type value);
udi_response_type udi_response_type_ntoh(udi_response_type value);
udi_event_type udi_event_type_hton(udi_event_type value);
udi_event_type udi_event_type_ntoh(udi_event_type value);
uint64_t udi_uint64_t_hton(uint64_t value);
uint64_t udi_uint64_t_ntoh(uint64_t value);

/* payload handling */

void free_event_internal(udi_event_internal *event);
void free_response(udi_response *resp);

udi_event_internal create_event_error(uint64_t thread_id, udi_errmsg *errmsg);
udi_event_internal create_event_unknown(uint64_t thread_id);
udi_event_internal create_event_breakpoint(uint64_t thread_id, udi_address bp_address);
udi_event_internal create_event_exit(uint64_t thread_id, uint32_t exit_status);
udi_event_internal create_event_thread_create(uint64_t thread_id);
udi_event_internal create_event_thread_death(uint64_t thread_id);
udi_event_internal create_event_fork(uint64_t thread_id, uint32_t pid);

int unpack_event_error(udi_event_internal *event, char **errmsg, unsigned int *errmsg_size);
int unpack_event_exit(udi_event_internal *event, int *exit_code);
int unpack_event_breakpoint(udi_event_internal *event, udi_address *addr);
int unpack_event_fork(udi_event_internal *event, uint32_t *pid);

udi_response create_response_error(udi_errmsg *errmsg);
udi_response create_response_read(const void *data, udi_length num_bytes);
udi_response create_response_init(udi_version_e protocol_version,
        udi_arch_e arch, int multithread, uint64_t tid);

typedef struct thread_state_struct {
    uint64_t tid;
    uint16_t state;
} thread_state;
udi_response create_response_state(int num_threads, thread_state *states);

int unpack_response_read(udi_response *resp, udi_length *num_bytes, void **value);
int unpack_response_error(udi_response *resp, udi_length *size, char **errmsg);
int unpack_response_init(udi_response *resp,  uint32_t *protocol_version,
        udi_arch_e *architecture, int *multithread_capable, uint64_t *tid);
int unpack_response_state(udi_response *resp, int *num_threads, thread_state **states);

int unpack_request_continue(udi_request *req, uint32_t *sig_val, udi_errmsg *errmsg);
int unpack_request_read(udi_request *req, udi_address *addr, udi_length *num_bytes, udi_errmsg *errmsg);
int unpack_request_write(udi_request *req, udi_address *addr, udi_length *num_bytes,
        void **bytes_to_write, udi_errmsg *errmsg);
int unpack_request_breakpoint_create(udi_request *req, udi_address *addr, udi_errmsg *errmsg);
int unpack_request_breakpoint(udi_request *req, udi_address *addr, udi_errmsg *errmsg);

udi_request create_request_breakpoint_create(udi_address addr);
udi_request create_request_breakpoint(udi_request_type request_type, udi_address addr);
udi_request create_request_read(udi_address addr, udi_length num_bytes);
udi_request create_request_write(udi_address addr, udi_length num_bytes, void *value);
udi_request create_request_continue(uint32_t sig_val);
udi_request create_request_state();
udi_request create_request_thr_state(udi_thread_state_e state);

#ifdef __cplusplus
} // extern C
#endif

#endif
