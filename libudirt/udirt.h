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

/* UDI implementation common between all platforms */

#ifndef _UDI_RT_H
#define _UDI_RT_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include "udi.h"

/* platform independent settings */
extern const char *UDI_ROOT_DIR_ENV;
extern const char *UDI_DEBUG_ENV;
extern char *UDI_ROOT_DIR;
extern int udi_debug_on;

extern const char *REQUEST_FILE_NAME;
extern const char *RESPONSE_FILE_NAME;
extern const char *EVENTS_FILE_NAME;

/* platform specific settings */

extern const char *UDI_DS;
extern const char *DEFAULT_UDI_ROOT_DIR;

/* platform independent structures */

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

/* platform specific functions */

void udi_free(void *ptr);
void *udi_malloc();

/* architecture specific functions */
uint64_t udi_unpack_uint64_t(uint64_t value);

/* platform independent functions */

udi_request *read_request();
int write_response(udi_response *response);
int write_event(udi_event *event);

udi_response *create_response(udi_response_type response_type,
                              udi_request_type request_type,
                              udi_length length,
                              void *value);
udi_event *create_event(udi_event_type event_type,
                        udi_length length,
                        void *data);
void free_request(udi_request *request);
void free_response(udi_response *response);
void free_event(udi_event *event);

/* error logging */
#define udi_printf(format, ...) \
    do {\
        if( udi_debug_on ) {\
            fprintf(stderr, "%s[%d]: " format, __FILE__, __LINE__,\
                    ## __VA_ARGS__);\
        }\
    }while(0)

#ifdef __cplusplus
} /* extern C */
#endif

#endif
