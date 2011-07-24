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

/* Definition of the userland debugger interface (UDI) */

/* UDI types */
typedef uint64_t udi_address;
typedef uint64_t udi_length;
typedef uint64_t udi_request_type;
typedef uint64_t udi_response_type;
typedef uint64_t udi_event_type;

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
    CONTINUE = 0,
    READ_MEM,
    WRITE_MEM,
    STATE
    INIT
} udi_request_type_e;

/*
 * Response specification
 *
 * +-------------------+------------------+------------+----------------+
 * | udi_response_type | udi_request_type | udi_length | void *value    |
 * +-------------------+------------------+------------+----------------+
 */
typedef enum
{
    ERROR = 0,
    VALID
} udi_response_type_e;

/* Request-response payload definitions 
 * 
 * For all request-reponse pairs:
 *      if udi_response_type == ERROR:
 *              value == error message
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
 *      udi_address - virtual memory address to read from
 *      udi_length - # of bytes to read
 *
 * Response values:
 *      requested memory values of target process
 */

/* Write request and response
 *
 * Request arguments:
 *      udi_address - virtual memory address to write to
 *      udi_length - # of bytes to write
 *      void * - values to write into target process memory
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
 * Events occur asynchronously -- that is they nevery directly occur in response to
 * requests.
 */

typedef enum
{
    ERROR = -1,
    SIGNAL
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
 * udi_length - length of error message
 * char * - error message
 */

/*
 * Signal event data
 *
 * udi_length - length of address field
 * udi_address - virtual address where the signal occurred
 */

#endif
