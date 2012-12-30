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

// Shared debugger and debuggee UDI implementation on all platforms

#include "udi.h"
#include "udi-common.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


const char *UDI_ROOT_DIR_ENV = "UDI_ROOT_DIR";
const char *REQUEST_FILE_NAME = "request";
const char *RESPONSE_FILE_NAME = "response";
const char *EVENTS_FILE_NAME = "events";
const char *UDI_DEBUG_ENV = "UDI_DEBUG";

#define CASE_TO_STR(x) case x: return #x

const char *request_type_str(udi_request_type req_type) {
    switch(req_type) {
        CASE_TO_STR(UDI_REQ_CONTINUE);
        CASE_TO_STR(UDI_REQ_READ_MEM);
        CASE_TO_STR(UDI_REQ_WRITE_MEM);
        CASE_TO_STR(UDI_REQ_STATE);
        CASE_TO_STR(UDI_REQ_INIT);
        CASE_TO_STR(UDI_REQ_MAX);
        default: return "UNKNOWN";
    }
}

const char *event_type_str(udi_event_type event_type) {
    switch(event_type) {
        CASE_TO_STR(UDI_EVENT_ERROR);
        CASE_TO_STR(UDI_EVENT_SIGNAL);
        CASE_TO_STR(UDI_EVENT_BREAKPOINT);
        CASE_TO_STR(UDI_EVENT_THREAD_CREATE);
        CASE_TO_STR(UDI_EVENT_THREAD_DEATH);
        CASE_TO_STR(UDI_EVENT_PROCESS_EXIT);
        CASE_TO_STR(UDI_EVENT_MAX);
        CASE_TO_STR(UDI_EVENT_UNKNOWN);
        default: return "UNSPECIFIED";
    }
}

/* network order packing */
extern uint64_t udi_unpack_uint64_t(uint64_t value);
extern uint32_t udi_unpack_uint32_t(uint32_t value);
extern uint16_t udi_unpack_uint16_t(uint16_t value);

udi_address udi_address_hton(udi_address value) {
    return udi_unpack_uint64_t(value);
}
udi_address udi_address_ntoh(udi_address value) {
    return udi_unpack_uint64_t(value);
}

udi_length udi_length_hton(udi_length value) {
    return udi_unpack_uint32_t(value);
}

udi_length udi_length_ntoh(udi_length value) {
    return udi_unpack_uint32_t(value);
}

udi_request_type udi_request_type_hton(udi_request_type value) {
    return udi_unpack_uint32_t(value);
}

udi_request_type udi_request_type_ntoh(udi_request_type value) {
    return udi_unpack_uint32_t(value);
}

udi_response_type udi_response_type_hton(udi_response_type value) {
    return udi_unpack_uint32_t(value);
}

udi_response_type udi_response_type_ntoh(udi_response_type value) {
    return udi_unpack_uint32_t(value);
}

udi_event_type udi_event_type_hton(udi_event_type value) {
    return udi_unpack_uint32_t(value);
}

udi_event_type udi_event_type_ntoh(udi_event_type value) {
    return udi_unpack_uint32_t(value);
}

uint64_t udi_uint64_t_hton(uint64_t value) {
    return udi_unpack_uint64_t(value);
}

uint64_t udi_uint64_t_ntoh(uint64_t value) {
    return udi_unpack_uint64_t(value);
}

static malloc_type data_allocator = malloc;

void udi_set_malloc(malloc_type allocator) {
    data_allocator = allocator;
}

/* 
 * The data format is dictated by the variable argument list.
 *
 * Formats for all types should be intuitive except UDI_TYPE_BYTESTREAM, which
 * is encoded as the size, followed by payload.
 *
 * Why the data types are not encoded in the data?
 *
 * There are two important use cases when retrieving data from a UDI-enabled 
 * process:
 * 
 * 1) The debugger requests a block of memory and knows how to interpret the
 *    block of memory without the inferior UDI code needing to interpret the
 *    data
 *
 * 2) The inferior UDI code passes back some structured data. This structure
 *    will be defined by the protocol.
 *
 * Therefore, there will never be a case where the debugger does not know the
 * structure of data coming from the UDI code.
 *
 * The benefits of this approach are that:
 * 1) The debugger always knows the structure of data and thus 
 *    alleviate the burden on the user for knowing the structure of data
 * 2) It saves bandwidth between the debugger and debuggee because type 
 *    information is not present.
 */

static
int udi_packer(int pack, unsigned char *data, udi_length length, va_list ap) {
    udi_length current_length = 0;

    int next_arg_is_type = 1;
    int next_arg_is_size = 0;
    udi_length next_arg_size = 0;
    udi_data_type last_type = UDI_DATATYPE_INT32;

    while ( current_length < length ) {
        if ( next_arg_is_type ) {
            last_type = va_arg(ap, udi_data_type);

            switch(last_type) {
                case UDI_DATATYPE_BYTESTREAM:
                    next_arg_is_size = 1;
                    next_arg_is_type = 0;
                    break;
                 default:
                    next_arg_is_size = 0;
                    next_arg_is_type = 0;
                    break;
            }

            switch(last_type) {
                case UDI_DATATYPE_INT16:
                    next_arg_size = sizeof(uint16_t);
                    break;
                case UDI_DATATYPE_LENGTH:
                case UDI_DATATYPE_INT32:
                    next_arg_size = sizeof(uint32_t);
                    break;
                case UDI_DATATYPE_ADDRESS:
                case UDI_DATATYPE_INT64:
                    next_arg_size = sizeof(uint64_t);
                    break;
                default:
                    break;
            }
        }else if (next_arg_is_size) {
            if ( pack ) {
                next_arg_size = va_arg(ap, udi_length);
                udi_length tmp_next_arg_size = udi_length_hton(next_arg_size);
                memcpy(data + current_length, &tmp_next_arg_size, 
                        sizeof(udi_length));
            }else{
                udi_length *buffer_size = va_arg(ap, udi_length *);
                memcpy(buffer_size, data + current_length, sizeof(udi_length));
                *buffer_size = udi_length_ntoh(*buffer_size);
                next_arg_size = *buffer_size;

                // Handle invalid bytestream size
                if (next_arg_size > (length - sizeof(udi_length))) {
                    // TODO this will cause a memory leak if mulitple
                    // bytestreams are used in the same unpack call
                    return -1;
                }
            }

            current_length += sizeof(udi_length);
            next_arg_is_size = 0;
            next_arg_is_type = 0;
        }else{
            if ( pack ) {
                switch(last_type) {
                    case UDI_DATATYPE_BYTESTREAM: 
                    {
                        void *arg = va_arg(ap, void *);
                        memcpy(data + current_length, arg, next_arg_size);
                        break;
                    }
                    case UDI_DATATYPE_INT16:
                    {
                        uint16_t arg = (uint16_t)va_arg(ap, uint32_t);
                        uint16_t tmp_arg = udi_unpack_uint16_t(arg);
                        memcpy(data + current_length, &tmp_arg, next_arg_size);
                        break;
                    }
                    case UDI_DATATYPE_LENGTH:
                    case UDI_DATATYPE_INT32:
                    {
                        uint32_t arg = va_arg(ap, uint32_t);
                        uint32_t tmp_arg = udi_unpack_uint32_t(arg);
                        memcpy(data + current_length, &tmp_arg, next_arg_size);
                        break;
                    }
                    case UDI_DATATYPE_ADDRESS:
                    case UDI_DATATYPE_INT64:
                    {
                        uint64_t arg = va_arg(ap, uint64_t);
                        uint64_t tmp_arg = udi_unpack_uint64_t(arg);
                        memcpy(data + current_length, &tmp_arg, next_arg_size);
                        break;
                    }
                    default:
                        break;
                }
            }else{
                if ( last_type != UDI_DATATYPE_BYTESTREAM ) {
                    void *output_data = va_arg(ap, void *);
                    memcpy(output_data, data + current_length, next_arg_size);

                    switch(last_type) {
                        case UDI_DATATYPE_INT16:
                            *((uint16_t *)output_data) = 
                                udi_unpack_uint16_t(*((uint16_t *)output_data));
                            break;
                        case UDI_DATATYPE_LENGTH:
                        case UDI_DATATYPE_INT32:
                            *((uint32_t *)output_data) = 
                                udi_unpack_uint32_t(*((uint32_t *)output_data));
                            break;
                        case UDI_DATATYPE_ADDRESS:
                        case UDI_DATATYPE_INT64:
                            *((uint64_t *)output_data) =
                                udi_unpack_uint64_t(*((uint64_t *)output_data));
                            break;
                        default:
                            break;
                    }
                }else{
                    void **output_data = va_arg(ap, void **);
                    *output_data = data_allocator(next_arg_size);
                    if ( *output_data == NULL ) {
                        // TODO this will cause a memory leak if multiple BYTESTREAM
                        // types in the same unpack call
                        return -1;
                    }

                    memcpy(*output_data, data + current_length, next_arg_size);
                }
            }

            current_length += next_arg_size;
            next_arg_is_size = 0;
            next_arg_is_type = 1;
        }
    }

    return 0;
}

void *udi_pack_data(udi_length length, ...) {
    void *result = data_allocator(length);
    if ( NULL == result ) return NULL;

    va_list ap;
    va_start(ap, length);

    if ( udi_packer(1, (unsigned char *)result, length, ap) ) {
        free(result);
        result = NULL;
    }

    va_end(ap);

    return result;
}

int udi_unpack_data(void *data, udi_length length, ...) {
    va_list ap;
    va_start(ap, length);

    int result = udi_packer(0, (unsigned char *)data, length, ap);

    va_end(ap);

    return result;
}
