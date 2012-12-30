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

// UDI message payload handling

#include "udi-common.h"

#include <string.h>
#include <stdio.h>

/**
 * Frees an internal event
 *
 * @param event the internal event
 */
void free_event_internal(udi_event_internal *event) {
    if ( event->packed_data != NULL ) free(event->packed_data);
    free(event);
}

/**
 * Frees the specified response
 *
 * @param resp the response
 */
void free_response(udi_response *resp) {
    if ( resp->packed_data != NULL ) free(resp->packed_data);
    free(resp);
}

/**
 * Creates an error event
 *
 * @param thread_id the thread id
 * @param errmsg the error message
 * @param errmsg_size the maximum size for the error message
 *
 * @return the created event
 */
udi_event_internal create_event_error(uint64_t thread_id, const char *errmsg, unsigned int errmsg_size) {
    udi_length payload_length = strnlen(errmsg, errmsg_size) + 1;

    udi_event_internal result;
    result.event_type = UDI_EVENT_ERROR;
    result.thread_id = thread_id;
    result.length = sizeof(udi_length) + payload_length;
    result.packed_data = udi_pack_data(result.length,
            UDI_DATATYPE_BYTESTREAM, payload_length, errmsg);

    return result;
}

/**
 * Unpacks the error event into specified parameters
 *
 * @param event the event to unpack
 * @param errmsg the error message to unpack into
 * @param errmsg_size the size of the error message
 *
 * @return 0 on success; non-zero otherwise
 */
int unpack_event_error(udi_event_internal *event, char **errmsg, unsigned int *errmsg_size) {

    if ( udi_unpack_data(event->packed_data, event->length,
                UDI_DATATYPE_BYTESTREAM, errmsg_size, errmsg) ) {
        return -1;
    }

    return 0;
}

/**
 * Unpacks the data from the exit event into the specified data
 *
 * @param event the exit event
 * @param exit_code the output parameter for the exit code
 *
 * @return 0 on success; non-zero otherwise
 */
int unpack_event_exit(udi_event_internal *event, int *exit_code) {

    if ( udi_unpack_data(event->packed_data, event->length,
                UDI_DATATYPE_INT32, exit_code) ) {
        return 1;
    }

    return 0;
}

/**
 * Unpacks the breakpoint event into the specified parameters
 *
 * @param event the breakpoint event
 * @param addr the output parameter for the address
 *
 * @return 0 on success; non-zero otherwise
 */
int unpack_event_breakpoint(udi_event_internal *event, udi_address *addr) {

    if ( udi_unpack_data(event->packed_data, event->length,
                UDI_DATATYPE_ADDRESS, addr) ) {
        return -1;
    }

    return 0;
}

/**
 * Creates a breakpoint event
 *
 * @param thread_id the thread id
 * @param bp_address the address of the breakpoint
 * 
 * @return the created event
 */
udi_event_internal create_event_breakpoint(uint64_t thread_id, udi_address bp_address) {
    
    udi_event_internal brkpt_event;
    brkpt_event.event_type = UDI_EVENT_BREAKPOINT;
    brkpt_event.thread_id = thread_id;
    brkpt_event.length = sizeof(udi_address);
    brkpt_event.packed_data = udi_pack_data(brkpt_event.length,
            UDI_DATATYPE_ADDRESS, bp_address);

    return brkpt_event;
}

/**
 * Creates an exit event
 *
 * @param thread_id the thread id
 * @param exit_status the status of the exit
 *
 * @return the created event
 */
udi_event_internal create_event_exit(uint64_t thread_id, uint32_t exit_status) {
    udi_event_internal exit_event;
    exit_event.event_type = UDI_EVENT_PROCESS_EXIT;
    exit_event.thread_id = thread_id;
    exit_event.length = sizeof(uint32_t);
    exit_event.packed_data = udi_pack_data(exit_event.length,
            UDI_DATATYPE_INT32, exit_status);

    return exit_event;
}

/**
 * Creates an thread create event
 *
 * @param thread_id the user level thread id for the thread that is about to be destroyed
 *
 * @return the created event
 */
udi_event_internal create_event_thread_create(uint64_t thread_id) {
    udi_event_internal thread_create;
    thread_create.event_type = UDI_EVENT_THREAD_CREATE;
    thread_create.thread_id = thread_id;
    thread_create.length = 0;

    return thread_create;
}

/**
 * Creates a thread death event
 *
 * @param tid the user level thread id for the thread that is about to be destroyed
 *
 * @return the created event
 */
udi_event_internal create_event_thread_death(uint64_t thread_id) {
    udi_event_internal thread_destroy;
    thread_destroy.event_type = UDI_EVENT_THREAD_DEATH;
    thread_destroy.thread_id = thread_id;
    thread_destroy.length = 0;

    return thread_destroy;
}

/**
 * Creates an unknown event
 *
 * @return the created event
 */
udi_event_internal create_event_unknown(uint64_t thread_id) {
    udi_event_internal result;
    result.event_type = UDI_EVENT_UNKNOWN;
    result.thread_id = thread_id;
    result.length = 0;
    result.packed_data = NULL;

    return result;
}

/**
 * Creates an error response
 *
 * @param errmsg the error message to be included in the response
 * @param errmsg_size the length of the error message
 *
 * @return the created response
 */
udi_response create_response_error(const char *errmsg, unsigned int errmsg_size) {
    udi_length payload_length = strnlen(errmsg, errmsg_size) + 1;

    udi_response error_resp;
    error_resp.response_type = UDI_RESP_ERROR;
    error_resp.request_type = UDI_REQ_INVALID;
    error_resp.length = sizeof(udi_length) + payload_length;
    error_resp.packed_data = udi_pack_data(error_resp.length,
            UDI_DATATYPE_BYTESTREAM, payload_length, errmsg);

    return error_resp;
}

/**
 * Creates a read memory response
 *
 * @param data the data read
 * @param num_bytes the number of bytes read
 *
 * @return the created read response
 */
udi_response create_response_read(const void *data, udi_length num_bytes) {
    udi_response read_resp;
    read_resp.response_type = UDI_RESP_VALID;
    read_resp.request_type = UDI_REQ_READ_MEM;
    read_resp.length = sizeof(udi_length) + num_bytes;
    read_resp.packed_data = udi_pack_data(read_resp.length, UDI_DATATYPE_BYTESTREAM,
            num_bytes, data);

    return read_resp;
}

/**
 * Creates an init response
 *
 * @param protocol_version version of the protocol being spoken
 * @param arch the architecture of the debuggee
 * @param multithread the multithread capability of the debuggee
 *
 * @return the created response
 */
udi_response create_response_init(udi_version_e protocol_version,
        udi_arch_e arch, int multithread)
{
    udi_response init_response;
    init_response.response_type = UDI_RESP_VALID;
    init_response.request_type = UDI_REQ_INIT;
    init_response.length = sizeof(uint32_t)*3;
    init_response.packed_data = udi_pack_data(init_response.length,
            UDI_DATATYPE_INT32, protocol_version,
            UDI_DATATYPE_INT32, arch,
            UDI_DATATYPE_INT32, multithread);

    return init_response;
}

/**
 * Unpacks the data contained in the continue request
 *
 * @param req the continue request
 * @param sig_val the output signal value used to continue the process
 * @param errmsg the error message populated on error
 * @param errmsg_size the size of the error message
 *
 * @return zero on success; non-zero otherwise
 */
int unpack_request_continue(udi_request *req, uint32_t *sig_val, char *errmsg, unsigned int errmsg_size) {
    int parsed = 1;
    do {
        if (req->request_type != UDI_REQ_CONTINUE) {
            parsed = 0;
            break;
        }

        if (udi_unpack_data(req->packed_data, req->length, 
               UDI_DATATYPE_INT32, sig_val)) {
            parsed = 0;
            break;
        }
    }while(0); 

    if (!parsed) {
        snprintf(errmsg, errmsg_size, "%s", "failed to parse continue request");
        return -1;
    }

    return 0;
}

/**
 * Unpacks the data from the read request
 *
 * @param req the request to parse
 * @param addr the output parameter for the address
 * @param num_bytes the output parameter for the number of bytes to read
 * @param errmsg the error message populated on error
 * @param errmsg_size the error message populated on size
 *
 * @return 0 on success; non-zero on failure
 */
int unpack_request_read(udi_request *req, udi_address *addr, 
        udi_length *num_bytes, char *errmsg, unsigned int errmsg_size) {

    if ( udi_unpack_data(req->packed_data, req->length,
                UDI_DATATYPE_ADDRESS, addr, UDI_DATATYPE_LENGTH,
                &num_bytes) )
    {
        snprintf(errmsg, errmsg_size, "%s", "failed to parse read request");
        return -1;
    }

    return 0;
}

/**
 * Unpacks the data from a write request
 *
 * @param req the request
 * @param addr the location of the write
 * @param num_bytes the number of bytes to write
 * @param bytes_to_write the data to write
 * @param errmsg the error message populated on failure
 * @param errmsg_size the size of the error message buffer
 *
 * @return -1 on failure; 0 otherwise
 */
int unpack_request_write(udi_request *req, udi_address *addr, udi_length *num_bytes,
        void **bytes_to_write, char *errmsg, unsigned int errmsg_size) {

    if ( udi_unpack_data(req->packed_data, req->length,
                UDI_DATATYPE_ADDRESS, addr, UDI_DATATYPE_BYTESTREAM, num_bytes,
                bytes_to_write) ) 
    {
        snprintf(errmsg, errmsg_size, "%s", "failed to parse write request");
        return -1;
    }

    return 0;
}

/**
 * Unpacks the data in a breakpoint create request
 *
 * @param req the request
 * @param addr the address of the breakpoint
 * @param errmsg the error message populated on error
 * @param errmsg_size the size of the error message
 *
 * @return 0 on success; -1 otherwise
 */
int unpack_request_breakpoint_create(udi_request *req, udi_address *addr,
        char *errmsg, unsigned int errmsg_size) {

    if (udi_unpack_data(req->packed_data, req->length,
                UDI_DATATYPE_ADDRESS, addr) ) {

        snprintf(errmsg, errmsg_size, "%s", "failed to parse breakpoint create request");
        return -1;
    }

    return 0;
}

/**
 * Unpack data for breakpoint related request
 *
 * @param req the request
 * @param addr the address of the request
 * @param errmsg the error message populated on failure
 * @param errmsg_size the size of the error message buffer
 *
 * @return 0 on success; non-zero otherwise
 */
int unpack_request_breakpoint(udi_request *req, udi_address *addr, char *errmsg, unsigned int errmsg_size) {

    if (udi_unpack_data(req->packed_data, req->length,
                UDI_DATATYPE_ADDRESS, addr)) 
    {
        snprintf(errmsg, errmsg_size, "%s", "failed to parse breakpoint request");
        return -1;
    }

    return 0;
}

/**
 * Creates a request to create a breakpoint
 *
 * @param addr the address for the breakpoint
 *
 * @return the created request
 */
udi_request create_request_breakpoint_create(udi_address addr) {

    udi_request request;
    request.request_type = UDI_REQ_CREATE_BREAKPOINT;
    request.length = sizeof(udi_address);
    request.packed_data = udi_pack_data(request.length,
            UDI_DATATYPE_ADDRESS, addr);

    return request;
}

/**
 * Creates a breakpoint request
 *
 * @param request_type the request type
 * @param addr the address of the breakpoint
 *
 * @return the created request
 */
udi_request create_request_breakpoint(udi_request_type request_type, udi_address addr) {
    
    udi_request request;
    request.request_type = request_type;
    request.length = sizeof(udi_address);
    request.packed_data = udi_pack_data(request.length, UDI_DATATYPE_ADDRESS, addr);

    return request;
}

/**
 * Creates a read request
 *
 * @param addr the address of the data to read
 * @param num_bytes the number of bytes to read
 *
 * @return the created request
 */
udi_request create_request_read(udi_address addr, udi_length num_bytes) {

    udi_request request;
    request.request_type = UDI_REQ_READ_MEM;
    request.length = sizeof(udi_length) + sizeof(udi_address);
    request.packed_data = udi_pack_data(request.length,
            UDI_DATATYPE_ADDRESS, addr, UDI_DATATYPE_LENGTH, num_bytes);

    return request;
}

/**
 * Creates a write request
 *
 * @param addr the target address for the data
 * @param num_bytes the number of bytes to write
 * @param value the data to write
 *
 * @return the created request
 */
udi_request create_request_write(udi_address addr, udi_length num_bytes, void *value) {

    udi_request request;
    request.request_type = UDI_REQ_WRITE_MEM;
    request.request_type = num_bytes + sizeof(udi_length) + sizeof(udi_address);
    request.packed_data = udi_pack_data(request.length,
            UDI_DATATYPE_ADDRESS, addr, UDI_DATATYPE_BYTESTREAM,
            num_bytes, value);

    return request;
}

/**
 * Creates a continue request
 *
 * @param sig_val the signal value to continue with, 0 for none
 * 
 * @return the created request
 */
udi_request create_request_continue(uint32_t sig_val) {

    udi_request req;
    req.request_type = UDI_REQ_CONTINUE;
    req.length = sizeof(uint32_t);
    req.packed_data = udi_pack_data(req.length,
            UDI_DATATYPE_INT32, sig_val);

    return req;
}

/**
 * Unpacks the read response
 *
 * @param resp the response
 * @param num_bytes the number of bytes read
 * @param values the data read
 *
 * @return 0 on success; non-zero on failure
 */
int unpack_response_read(udi_response *resp, udi_length *num_bytes, void **value) {

    if (udi_unpack_data(resp->packed_data, resp->length,
                UDI_DATATYPE_BYTESTREAM, num_bytes, value) ) {
        return -1;
    }

    return 0;
}

/**
 * Unpacks the error response into the specified parameters
 *
 * @param resp the response
 * @param size the number of bytes in the error message
 * @param errmsg the error message output parameter
 *
 * @return 0 on success; non-zero on failure
 */
int unpack_response_error(udi_response *resp, udi_length *size, char **errmsg) {

    if (udi_unpack_data(resp->packed_data, resp->length,
                UDI_DATATYPE_BYTESTREAM, size, errmsg)) {
        return -1;
    }

    return 0;
}

/**
 * Unpacks the init response
 *
 * @param resp the response
 * @param protocol_version the protocol version output parameter
 * @param architecture the architecture output parameter
 * @param multithread_capable the multithread capabable output parameter
 *
 * @return 0 on success; non-zero on failure
 */
int unpack_response_init(udi_response *resp,  uint32_t *protocol_version,
        udi_arch_e *architecture, int *multithread_capable) {

    if (udi_unpack_data(resp->packed_data, resp->length,
                UDI_DATATYPE_INT32, protocol_version,
                UDI_DATATYPE_INT32, architecture,
                UDI_DATATYPE_INT32, multithread_capable)) {
        return -1;
    }

    return 0;
}
