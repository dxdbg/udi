/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
const char *arch_str(udi_arch_e arch);
const char *register_str(udi_register_e reg);

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
udi_event_internal create_event_thread_create(uint64_t creator_id, uint64_t new_id);
udi_event_internal create_event_thread_death(uint64_t thread_id);
udi_event_internal create_event_fork(uint64_t thread_id, uint32_t pid);
udi_event_internal create_event_single_step(uint64_t thread_id);

int unpack_event_error(udi_event_internal *event, char **errmsg, unsigned int *errmsg_size);
int unpack_event_exit(udi_event_internal *event, int *exit_code);
int unpack_event_thread_create(udi_event_internal *event, uint64_t *new_thr_id);
int unpack_event_breakpoint(udi_event_internal *event, udi_address *addr);
int unpack_event_fork(udi_event_internal *event, uint32_t *pid);

udi_response create_response_error(udi_errmsg *errmsg);
udi_response create_response_read(const void *data, udi_length num_bytes);
udi_response create_response_init(udi_version_e protocol_version,
        udi_arch_e arch, int multithread, uint64_t tid);
udi_response create_response_read_register(udi_address value);
udi_response create_response_next_instr(udi_address value);
udi_response create_response_single_step(uint16_t setting);

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
int unpack_response_read_register(udi_response *resp, udi_address *value);
int unpack_response_next_instr(udi_response *resp, udi_address *value);
int unpack_response_single_step(udi_response *resp, uint16_t *setting);

int unpack_request_continue(udi_request *req, uint32_t *sig_val, udi_errmsg *errmsg);
int unpack_request_read(udi_request *req, udi_address *addr, udi_length *num_bytes, udi_errmsg *errmsg);
int unpack_request_write(udi_request *req, udi_address *addr, udi_length *num_bytes,
        void **bytes_to_write, udi_errmsg *errmsg);
int unpack_request_breakpoint_create(udi_request *req, udi_address *addr, udi_errmsg *errmsg);
int unpack_request_breakpoint(udi_request *req, udi_address *addr, udi_errmsg *errmsg);
int unpack_request_read_register(udi_request *req, uint32_t *reg, udi_errmsg *errmsg);
int unpack_request_write_register(udi_request *req, uint32_t *reg, udi_address *value, udi_errmsg *errmsg);
int unpack_request_single_step(udi_request *req, uint16_t *setting, udi_errmsg *errmsg);

udi_request create_request_breakpoint_create(udi_address addr);
udi_request create_request_breakpoint(udi_request_type request_type, udi_address addr);
udi_request create_request_read(udi_address addr, udi_length num_bytes);
udi_request create_request_write(udi_address addr, udi_length num_bytes, void *value);
udi_request create_request_continue(uint32_t sig_val);
udi_request create_request_state();
udi_request create_request_thr_state(udi_thread_state_e state);
udi_request create_request_read_reg(udi_register_e reg);
udi_request create_request_write_reg(udi_register_e reg, udi_address value);
udi_request create_request_next_instr();
udi_request create_request_single_step(uint16_t setting);

#ifdef __cplusplus
} // extern C
#endif

#endif
