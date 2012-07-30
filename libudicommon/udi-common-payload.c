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
 * Creates an error event
 *
 * @param errmsg the error message
 * @param errmsg_size the maximum size for the error message
 *
 * @return the created event
 */
udi_event_internal create_event_error(const char *errmsg, unsigned int errmsg_size) {
    udi_length payload_length = strnlen(errmsg, errmsg_size) + 1;

    udi_event_internal result;
    result.event_type = UDI_EVENT_ERROR;
    result.length = sizeof(udi_length) + payload_length;
    result.packed_data = udi_pack_data(result.length,
            UDI_DATATYPE_BYTESTREAM, payload_length, errmsg);

    return result;
}

/**
 * Creates a breakpoint event
 *
 * @param bp_address the address of the breakpoint
 * 
 * @return the created event
 */
udi_event_internal create_event_breakpoint(udi_address bp_address) {
    
    udi_event_internal brkpt_event;
    brkpt_event.event_type = UDI_EVENT_BREAKPOINT;
    brkpt_event.length = sizeof(udi_address);
    brkpt_event.packed_data = udi_pack_data(brkpt_event.length,
            UDI_DATATYPE_ADDRESS, bp_address);

    return brkpt_event;
}

/**
 * Creates an exit event
 *
 * @param exit_status the status of the exit
 *
 * @return the created event
 */
udi_event_internal create_event_exit(uint32_t exit_status) {
    udi_event_internal exit_event;
    exit_event.event_type = UDI_EVENT_PROCESS_EXIT;
    exit_event.length = sizeof(uint32_t);
    exit_event.packed_data = udi_pack_data(exit_event.length,
            UDI_DATATYPE_INT32, exit_status);

    return exit_event;
}

/**
 * Creates an unknown event
 *
 * @return the created event
 */
udi_event_internal create_event_unknown() {
    udi_event_internal result;
    result.event_type = UDI_EVENT_UNKNOWN;
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

    if (*sig_val < 0) {
        snprintf(errmsg, errmsg_size, "invalid signal specified: %d", *sig_val);
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
