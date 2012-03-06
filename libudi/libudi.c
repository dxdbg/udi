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

#include "udi.h"
#include "libudi.h"
#include "udi-common.h"
#include "libudi-private.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

////////////////////
// globals        //
////////////////////
int udi_debug_on = 0;
int processes_created = 0;
const char *udi_root_dir = NULL;

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

int init_libudi() {
    if ( udi_root_dir == NULL ) {
        udi_root_dir = DEFAULT_UDI_ROOT_DIR;
    }

    check_debug_logging();

    return 0;
}

udi_process *create_process(const char *executable, char * const argv[],
        char * const envp[])
{
    int error = 0;

    // Validate arguments
    if (argv == NULL || executable == NULL) {
        udi_printf("%s\n", "invalid arguments");
        return NULL;
    }

    if ( processes_created == 0 ) {
        if ( create_root_udi_filesystem() != 0 ) {
            udi_printf("%s\n", "failed to create root UDI filesystem");
            return NULL;
        }
    }

    if (envp == NULL) {
        // Created process will inherit the current environment
        envp = get_environment();
    }

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
    }else{
        processes_created++;
    }

    return proc;
}

int free_process(udi_process *proc) {
    // TODO detach from the process, etc.
    free(proc);

    return 1;
}

int set_udi_root_dir(const char *root_dir) {
    if ( processes_created > 0 ) return -1;

    size_t length = strlen(root_dir);

    udi_root_dir = (char *)malloc(length);

    if (udi_root_dir == NULL) {
        return -1;
    }

    strncpy((char *)udi_root_dir, root_dir, length);

    return 0;
}

void set_user_data(udi_process *proc, void *user_data) {
    proc->user_data = user_data;
}

void *get_user_data(udi_process *proc) {
    return proc->user_data;
}

int get_proc_pid(udi_process *proc) {
    return proc->pid;
}

static
udi_error_e submit_request(udi_process *proc,
        udi_request *req, udi_response **resp,
        const char *desc, const char *file, 
        int line)
{
    udi_error_e error_code = UDI_ERROR_NONE;

    do {
        // Perform the request
        if ( req->packed_data == NULL ) {
            udi_printf("failed to pack data for %s\n", desc);
            error_code = UDI_ERROR_LIBRARY;
            break;
        }

        if ( write_request(req, proc) != 0 ) {
            udi_printf("failed to write %s\n", desc);
            error_code = UDI_ERROR_LIBRARY;
            break;
        }

        // Get the response
        *resp = read_response(proc);

        if ( *resp == NULL ) {
            udi_printf("failed to read response for %s\n", desc);
            error_code = UDI_ERROR_LIBRARY;
            break;
        }

        if ( (*resp)->response_type == UDI_RESP_ERROR ) {
            log_error_msg(*resp, file, line);
            error_code = UDI_ERROR_REQUEST;
            break;
        }
    }while(0);

    if ( req->packed_data != NULL ) free(req->packed_data);
    if (    error_code != UDI_ERROR_NONE
         && *resp != NULL ) 
    {
        free_response(*resp);
        *resp = NULL;
    }

    return error_code;
}

static
udi_error_e submit_request_noresp(udi_process *proc,
        udi_request *req, const char *desc, const char *file,
        int line)
{
    udi_response *resp = NULL;

    udi_error_e error_code = submit_request(proc, req, &resp, desc,
            file, line);

    if ( resp != NULL ) free_response(resp);

    return error_code;
}

udi_error_e create_breakpoint(udi_process *proc, udi_address addr,
        udi_length instr_length)
{
    udi_request request;

    request.request_type = UDI_REQ_CREATE_BREAKPOINT;
    request.length = sizeof(udi_length) + sizeof(udi_address);
    request.packed_data = udi_pack_data(request.length,
            UDI_DATATYPE_ADDRESS, addr, UDI_DATATYPE_LENGTH,
            instr_length);

    return submit_request_noresp(proc, &request, "breakpoint create request",
            __FILE__, __LINE__);
}

static
udi_error_e breakpoint_request(udi_process *proc, udi_address addr,
        udi_request_type request_type,
        const char *desc, const char *file, int line)
{

    udi_request request;
    request.request_type = request_type;
    request.length = sizeof(udi_address);
    request.packed_data = udi_pack_data(request.length,
            UDI_DATATYPE_ADDRESS, addr);

    return submit_request_noresp(proc, &request, desc, file, line);
}

udi_error_e install_breakpoint(udi_process *proc, udi_address addr) {
    return breakpoint_request(proc, addr, UDI_REQ_INSTALL_BREAKPOINT,
            "breakpoint install request", __FILE__, __LINE__);
}

udi_error_e remove_breakpoint(udi_process *proc, udi_address addr) {
    return breakpoint_request(proc, addr, UDI_REQ_REMOVE_BREAKPOINT,
            "breakpoint remove request", __FILE__, __LINE__);

}

udi_error_e delete_breakpoint(udi_process *proc, udi_address addr) {
    return breakpoint_request(proc, addr, UDI_REQ_DELETE_BREAKPOINT,
            "breakpoint delete request", __FILE__, __LINE__);
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

    udi_response *resp = NULL;
    do{
        error_code = submit_request(proc, &request, &resp, "memory access request",
                __FILE__, __LINE__);

        if ( error_code != UDI_ERROR_NONE ) return error_code;

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

    if ( resp != NULL ) free_response(resp);

    return error_code;
}

udi_error_e continue_process(udi_process *proc) {
    udi_request req;
    req.request_type = UDI_REQ_CONTINUE;
    req.length = sizeof(uint32_t);
    req.packed_data = udi_pack_data(req.length,
            UDI_DATATYPE_INT32, 0);

    return submit_request_noresp(proc, &req, "continue request",
            __FILE__, __LINE__);
}

void log_error_msg(udi_response *resp, const char *error_file, int error_line) {
    char *errmsg_local;
    udi_length errmsg_size;

    if (resp->response_type != UDI_RESP_ERROR ) return;

    if ( udi_unpack_data(resp->packed_data, resp->length, 
                UDI_DATATYPE_BYTESTREAM, &errmsg_size, &errmsg_local) )
    {
        udi_printf("%s\n", "failed to unpack response for failed request");
    }else{
        udi_printf("request failed @[%s:%d]: %s\n", error_file, error_line, 
                errmsg_local);
        strncpy(errmsg, errmsg_local, ERRMSG_SIZE);
    }

    if ( errmsg_local != NULL ) free(errmsg_local);
}

static
void free_event(udi_event *event) {
    if (event->event_data != NULL ) {
        switch(event->event_type) {
            case UDI_EVENT_ERROR: 
            {
                const char *errstr = 
                    ((udi_event_error *)event->event_data)->errstr;

                if (errstr != NULL) free((void *)errstr);
                break;
            }
            default:
                break;
        }

        free(event->event_data);
    }

    free(event);
}

void free_event_list(udi_event *event_list) {
    udi_event *current_event = event_list;
    while ( current_event != NULL ) {
        udi_event *next_event = current_event->next_event;
        free_event(current_event);
        current_event = next_event;
    }
}

void free_response(udi_response *resp) {
    if ( resp->packed_data != NULL ) free(resp->packed_data);
    free(resp);
}

void free_event_internal(udi_event_internal *event) {
    if ( event->packed_data != NULL ) free(event->packed_data);
    free(event);
}

const char *get_event_type_str(udi_event_type event_type) {
    return event_type_str(event_type);
}

udi_event *decode_event(udi_process *proc, udi_event_internal *event) {
    udi_event *ret_event = (udi_event *)malloc(sizeof(udi_event));

    if ( ret_event == NULL ) {
        udi_printf("failed to malloc udi_event: %s\n", strerror(errno));
        return NULL;
    }

    ret_event->proc = proc;
    ret_event->next_event = NULL;
    ret_event->event_type = event->event_type;
    ret_event->event_data = NULL;

    switch(ret_event->event_type) {
        case UDI_EVENT_ERROR:
        {
            udi_event_error *err_event = (udi_event_error *)
                malloc(sizeof(udi_event_error));

            if (err_event == NULL) {
                udi_printf("failed to malloc error event: %s\n",
                        strerror(errno));
                free_event(ret_event);
                return NULL;
            }

            udi_length errmsg_size;

            if ( udi_unpack_data(event->packed_data, event->length,
                        UDI_DATATYPE_BYTESTREAM, &errmsg_size, 
                        err_event->errstr) )
            {
                udi_printf("%s\n", "failed to decode error event");

                free(err_event);
                free_event(ret_event);
                return NULL;
            }
            ret_event->event_data = err_event;
            break;
        }
        case UDI_EVENT_PROCESS_EXIT:
        {
            udi_event_process_exit *exit_event = (udi_event_process_exit *)
                malloc(sizeof(udi_event_process_exit));

            if (exit_event == NULL ) {
                udi_printf("failed to malloc exit event: %s\n",
                        strerror(errno));
                free_event(ret_event);
                return NULL;
            }

            if ( udi_unpack_data(event->packed_data, event->length,
                        UDI_DATATYPE_INT32, &(exit_event->exit_code)) )
            {
                udi_printf("%s\n", "failed to decode exit event");
                
                free(exit_event);
                free_event(ret_event);
                return NULL;
            }

            ret_event->event_data = exit_event;
            break;
        }
        case UDI_EVENT_BREAKPOINT:
        {
            udi_event_breakpoint *break_event = (udi_event_breakpoint *)
                malloc(sizeof(udi_event_breakpoint));

            if ( break_event == NULL ) {
                udi_printf("failed to malloc breakpoint event: %s\n",
                        strerror(errno));
                free_event(ret_event);
                return NULL;
            }

            if ( udi_unpack_data(event->packed_data, event->length,
                        UDI_DATATYPE_ADDRESS, &(break_event->breakpoint_addr)) )
            {
                udi_printf("%s\n", "failed to decode breakpoint event");
                free(break_event);
                free_event(ret_event);
                return NULL;
            }
            ret_event->event_data = break_event;
            break;
        }
        default:
            udi_printf("unknown event: %s\n", 
                    event_type_str(ret_event->event_type));
            free_event(ret_event);
            return NULL;
    }

    return ret_event;
}
