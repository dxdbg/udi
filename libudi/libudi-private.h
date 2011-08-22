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

#ifndef _LIBUDI_PRIVATE_H
#define _LIBUDI_PRIVATE_H 1

#include "libudi.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO make types cross platform
typedef int udi_handle;
typedef pid_t udi_pid;
const pid_t INVALID_UDI_PID = -1;

typedef enum {
    UDI_ARCH_X86,
    UDI_ARCH_X86_64
} udi_arch_e;

struct udi_process_struct {
    udi_pid pid;
    udi_handle request_handle;
    udi_handle response_handle;
    udi_handle events_handle;
    udi_arch_e architecture;
};

udi_pid fork_process(const char *executable, const char *argv[],
        const char *envp[]);

int initialize_process(udi_process *proc);

int write_request(udi_request *req);

udi_response *read_response();
void free_response(udi_response *resp);

// error logging
extern int udi_debug_on;

#define udi_printf(format, ...) \
    do {\
        if ( udi_debug_on ) {\
            fprintf(stderr, "%s[%d]: " format, __FILE__, __LINE__,\
                        ## __VA_ARGS__);\
        }\
    }while(0)

#ifdef __cplusplus
} // "C"
#endif

#endif
