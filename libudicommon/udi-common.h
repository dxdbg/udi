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

/* shared functionality between UDI debugger and debuggee */

#ifndef _UDI_COMMON_H
#define _UDI_COMMON_H 1

#include "udi.h"

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

/* platform specific variables */
extern const char *DEFAULT_UDI_ROOT_DIR;
extern const char *UDI_DS;
extern const unsigned int DS_LEN;

/*
 * data structure
 */
typedef struct {
    udi_response_type response_type;
    udi_request_type request_type;
    udi_length length;
    void *value;
} udi_response;

typedef struct {
    udi_request_type request_type;
    udi_length length;
    void *argument;
} udi_request;

typedef struct {
    udi_event_type event_type;
    udi_length length;
    void *data;
} udi_event;

/* helper functions */

uint64_t udi_unpack_uint64_t(uint64_t value);

const char *request_type_str(udi_request_type req_type); 

#ifdef __cplusplus
} // extern C
#endif

#endif
