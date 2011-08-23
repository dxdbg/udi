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

#include "udi.h"
#include "libudi.h"
#include "udi-common.h"
#include "libudi-private.h"

#include <stdlib.h>
#include <string.h>

////////////////////
// globals        //
////////////////////
int udi_debug_on = 0;

/////////////////////
// Error reporting //
/////////////////////
const unsigned int ERRMSG_SIZE = 4096;
char errmsg[4096];

const char *get_error_message(udi_error_e error_code) 
{
    switch(error_code) {
        case UDI_ERROR_LIBRARY:
            return "UDI library internal error";
        case UDI_ERROR_REQUEST:
            return errmsg;
        default:
            return "Error not set";
    }
}

///////////////////////
// Library functions //
///////////////////////

udi_process *create_process(const char *executable, const char *argv[],
        const char *envp[])
{
    int error = 0;

    udi_process *proc = (udi_process *)malloc(sizeof(udi_process));
    do{
        if ( proc == NULL ) {
            udi_printf("%s\n", "malloc failed");
            error = 1;
            break;
        }

        proc->pid = fork_process(executable, argv, envp);
        if ( proc->pid == INVALID_UDI_PID ) {
            udi_printf("failed to create process for executable %s\n",
                    executable);
            error = 1;
            break;
        }

        if ( initialize_process(proc) != 0 ) {
            udi_printf("%s\n", "failed to initialize process for debugging");
            error = 1;
            break;
        }
    }while(0);

    if ( error ) {
        if ( proc ) {
            free(proc);
            proc = NULL;
        }
    }

    return proc;
}

udi_error_e mem_access(udi_process *proc, int write, void *value, udi_length size, 
        udi_address addr) 
{
    udi_error_e error_code = UDI_ERROR_NONE;
    udi_request request;
    
    // Create the request
    if ( write ) {
        request.request_type = UDI_REQ_WRITE_MEM;
        request.length = size + sizeof(udi_length) + sizeof(udi_address);
        request.packed_data = udi_pack_data(request.length,
                UDI_DATATYPE_ADDRESS, addr, UDI_DATATYPE_BYTESTREAM,
                size, value);
    }else{
        request.request_type = UDI_REQ_READ_MEM;
        request.length = sizeof(udi_length) + sizeof(udi_address);
        request.packed_data = udi_pack_data(request.length, 
                UDI_DATATYPE_ADDRESS, addr, UDI_DATATYPE_LENGTH, size);
    }

    int errnum = 0;
    udi_response *resp = NULL;
    do{
        // Perform the request
        if ( request.packed_data == NULL ) {
            udi_printf("%s\n", "failed to pack data for memory access request");
            error_code = UDI_ERROR_LIBRARY;
            break;
        }

        if ( (errnum = write_request(&request, proc)) != 0 ) {
            udi_printf("%s\n", "failed to perform memory access request");
            error_code = UDI_ERROR_LIBRARY;
            break;
        }

        // Get the response
        resp = read_response(proc);
        if ( resp == NULL ) {
            udi_printf("%s\n", 
                    "failed to receive response for memory access request");
            error_code = UDI_ERROR_LIBRARY;
            break;
        }

        if ( resp->response_type == UDI_RESP_ERROR ) {
            char *errmsg_local;
            udi_length errmsg_size;

            if ( udi_unpack_data(resp->packed_data, resp->length, 
                        UDI_DATATYPE_BYTESTREAM, &errmsg_size, &errmsg_local) )
            {
                udi_printf("%s\n", "failed to unpack response for failed request");
                error_code = UDI_ERROR_LIBRARY;
                break;
            }

            udi_printf("request failed: %s\n", errmsg_local);
            strncpy(errmsg, errmsg_local, ERRMSG_SIZE);
            free(errmsg_local);
            error_code = UDI_ERROR_REQUEST;
            break;
        }

        if ( resp->request_type == UDI_REQ_READ_MEM ) {
            void *result;
            udi_length result_size;

            if ( udi_unpack_data(resp->packed_data, resp->length,
                        UDI_DATATYPE_BYTESTREAM, &result_size, &result) )
            {
                udi_printf("%s\n", "failed to unpack response for read request");
                error_code = UDI_ERROR_LIBRARY;
                break;
            }

            memcpy(value, result, ( result_size > size ) ? size : result_size);
            free(result);
        }

        // There are no values to parse for a valid write request
    }while(0);

    if ( request.packed_data != NULL ) free(request.packed_data);
    if ( resp != NULL ) free_response(resp);

    return error_code;
}

static const unsigned char X86_TRAP_INSTRUCT = 0xcc;

udi_error_e set_breakpoint(udi_process *proc, udi_address breakpoint_addr) 
{
    switch(proc->architecture) {
        case UDI_ARCH_X86:
        case UDI_ARCH_X86_64: 
        {
            unsigned char trap_instruction = X86_TRAP_INSTRUCT;
            return mem_access(proc, 1, &trap_instruction, 1, breakpoint_addr);
        }
        default:
             break;
    }

    return UDI_ERROR_NONE;
}

void free_response(udi_response *resp) {
    if ( resp->packed_data != NULL ) free(resp->packed_data);
    free(resp);
}
