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

#ifndef _UDI_H
#define _UDI_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Definition of the userland debugger interface (UDI)

// UDI types
typedef uint64_t udi_address;
typedef uint32_t udi_length;
typedef uint32_t udi_request_type;
typedef uint32_t udi_response_type;
typedef uint32_t udi_event_type;
typedef uint32_t udi_data_type;

/*
 * UDI data type
 */
typedef enum {
    UDI_DATATYPE_INT16 = 0,
    UDI_DATATYPE_INT32,
    UDI_DATATYPE_LENGTH,
    UDI_DATATYPE_INT64,
    UDI_DATATYPE_ADDRESS,
    UDI_DATATYPE_BYTESTREAM // encoded as a length and a following byte stream
} udi_data_type_e;

/* 
 * Request specification:
 *
 * +------------------+------------+----------------+
 * | udi_request_type | udi_length | void *argument |
 * +------------------+------------+----------------+
 *
 */
typedef enum
{
    UDI_REQ_CONTINUE = 0,
    UDI_REQ_READ_MEM,
    UDI_REQ_WRITE_MEM,
    UDI_REQ_STATE,
    UDI_REQ_INIT,
    UDI_REQ_MAX,
    UDI_REQ_INVALID,
} udi_request_type_e;

/*
 * Response specification
 *
 * +-------------------+------------------+------------+---------------------+
 * | udi_response_type | udi_request_type | udi_length | void *packed_data   |
 * +-------------------+------------------+------------+---------------------+
 */
typedef enum
{
    UDI_RESP_ERROR = 0,
    UDI_RESP_VALID,
    UDI_RESP_MAX
} udi_response_type_e;

/* Request-response payload definitions 
 * 
 * For all request-reponse pairs:
 *      if udi_response_type == ERROR:
 *              packed_data == error message (of type UDI_DATATYPE_BYTESTREAM)
 */

/* Init request and response
 *
 * Request arguments:
 *      None
 *
 * Response arguments:
 *      None
 */

/* Continue request and response
 *
 * Request arguments:
 *      None
 *
 * Response values:
 *      None
 */

/* Read request and response
 *
 * Request arguments:
 *      UDI_DATATYPE_ADDRESS - virtual memory address to read from
 *      UDI_DATATYPE_LENGTH - # of bytes to read
 *
 * Response values:
 *      UDI_DATATYPE_BYTESTREAM - requested memory values of target process
 */

/* Write request and response
 *
 * Request arguments:
 *      UDI_DATATYPE_ADDRESS - virtual memory address to write to
 *      UDI_DATATYPE_BYTESTREAM - bytes to write into target process
 *
 * Response values:
 *      None
 */

/* State request and response
 *
 * Request arguments:
 *      None
 *
 * Response values:
 *      TBD
 */

/* Event specification
 *
 * Events occur asynchronously 
 * -- that is they nevery directly occur in response to requests.
 */

typedef enum
{
    UDI_EVENT_ERROR = -1,
    UDI_EVENT_BREAKPOINT,
    UDI_EVENT_SIGNAL,
    UDI_EVENT_MAX,
    UDI_EVENT_UNKNOWN
} udi_event_type_e;

/*
 * Event data
 *
 * +----------------+------------+------------+
 * | udi_event_type | udi_length | void *data |
 * +----------------+------------+------------+
 */

/*
 * Error event data
 *
 * UDI_DATATYPE_BYTESTREAM - error message
 */

/*
 * Signal event data
 *
 * UDI_DATATYPE_ADDRESS - virtual address where the signal occurred
 */

/*
 * Breakpoint event data
 *
 * UDI_DATATYPE_ADDRESS - virtual addresss where the breakpoint occurred
 */

#ifdef __cplusplus
} // extern C
#endif

#endif
