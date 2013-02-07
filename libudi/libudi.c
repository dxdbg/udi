/*
 * Copyright (c) 2011-2013, Dan McNulty
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

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

/**
 * Get a message describing the passed error code
 *
 * Note: this may be dependent on the result of the most recent operation
 *
 * @param error_code the error code
 */
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

/**
 * Finds the thread structure for the specified tid
 *
 * @param proc  the process handle
 * @param tid   the thread id
 *
 * @return the thread structure or NULL if no thread could be found
 */
static
udi_thread *find_thread(udi_process *proc, udi_tid tid) {

    udi_thread *thr = proc->threads;
    while (thr != NULL) {
        if ( thr->tid == tid ) break;
        thr = thr->next_thread;
    }

    return thr;
}

/**
 * Initializes the library
 *
 * @return UDI_ERROR_NONE on success
 */
udi_error_e init_libudi() {
    if ( udi_root_dir == NULL ) {
        udi_root_dir = DEFAULT_UDI_ROOT_DIR;
    }

    check_debug_logging();

    return UDI_ERROR_NONE;
}

/**
 * Create UDI-controlled process
 * 
 * @param executable   the full path to the executable
 * @param argv         the arguments
 * @param envp         the environment, if NULL, the newly created process will
 *                     inherit the environment for this process
 *
 * @return a handle to the created process
 *
 * @see execve on a UNIX system
 */
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
    memset(proc, 0, sizeof(udi_process));
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

/**
 * Tells the library that resources allocated for the process can be released
 *
 * @param proc          the process handle
 *
 * @return UDI_ERROR_NONE if the resources are released successfully
 */
udi_error_e free_process(udi_process *proc) {
    // TODO detach from the process, etc.
    free(proc);

    return UDI_ERROR_NONE;
}

/**
 * Sets the directory to be used for the root of the UDI filesystem
 * 
 * Will be created if it doesn't exist
 * 
 * Cannot be set if processes already created
 *
 * @param root_dir the directory to set as the root
 *
 * @return UDI_ERROR_NONE on success
 */
udi_error_e set_udi_root_dir(const char *root_dir) {
    if ( processes_created > 0 ) return -1;

    size_t length = strlen(root_dir);

    udi_root_dir = (char *)malloc(length);
    if (udi_root_dir == NULL) {
        return UDI_ERROR_LIBRARY;
    }

    strncpy((char *)udi_root_dir, root_dir, length);

    return UDI_ERROR_NONE;
}

/**
 * Sets the user data stored with the internal process structure
 *
 * @param proc          the process handle
 * @param user_data     the user data to associated with the process handle
 */
void set_user_data(udi_process *proc, void *user_data) {
    proc->user_data = user_data;
}

/**
 * Gets the user data stored with the internal process structure
 *
 * @param proc          the process handle
 *
 * @return the user data
 */
void *get_user_data(udi_process *proc) {
    return proc->user_data;
}

/**
 * Gets the process identifier for the specified process
 *
 * @param proc          the process handle
 *
 * @return the pid for the process
 */
int get_proc_pid(udi_process *proc) {
    return proc->pid;
}

/**
 * Gets the architecture for the specified process
 *
 * @param proc          the process handle
 *
 * @return the architecture for the process
 */
udi_arch_e get_proc_architecture(udi_process *proc) {
    return proc->architecture;
}

/**
 * Gets whether the specified process is multithread capable
 *
 * @param proc          the process handle
 *
 * @return non-zero if the process is multithread capable
 */
int get_multithread_capable(udi_process *proc) {
    return proc->multithread_capable;
}

/**
 * Gets the thread id for the specified thread
 *
 * @param thr the thread handle
 *
 * @return the thread id for the thread
 */
uint64_t get_tid(udi_thread *thr) {
    return thr->tid;
}

/**
 * Gets the process for the specified thread
 *
 * @param thr the thread handle
 *
 * @return the process handle
 */
udi_process *get_process(udi_thread *thr) {
    return thr->proc;
}

/**
 * Gets the state for the specified thread
 *
 * @param thr thre thread handle
 *
 * @return the thread handle
 */
udi_thread_state_e get_state(udi_thread *thr) {
    return thr->state;
}

/*
 * Gets the initial thread in the specified process
 *
 * @param proc the process handle
 *
 * @return the thread handle or NULL if no initial thread could be found
 */
udi_thread *get_initial_thread(udi_process *proc) {
    udi_thread *iter = proc->threads;

    while (iter != NULL) {
        if (iter->initial) break;

        iter = iter->next_thread;
    }

    return iter;
}

/**
 * Gets the next thread
 * 
 * @param thr the thread
 *
 * @return the thread or NULL if no more threads
 */
udi_thread *get_next_thread(udi_thread *thr) {
    return thr->next_thread;
}

/**
 * Submits the specified request to the specified process
 *
 * @param proc          the process handle
 * @param req           the request
 * @param resp          the resulting response
 * @param desc          the description of the request
 * @param file          the file of the call to this function
 * @param line          the line of the call to this function
 *
 * @return the result of the operation, if UDI_ERROR_NONE then
 * the resp parameter will be populated with the response
 */
static
udi_error_e submit_request(udi_process *proc,
        udi_request *req, udi_response **resp,
        const char *desc, const char *file, 
        int line)
{
    udi_error_e error_code = UDI_ERROR_NONE;

    do {
        // Perform the request
        if ( req->request_type != UDI_REQ_STATE ) {
            if ( req->packed_data == NULL ) {
                udi_printf("failed to pack data for %s\n", desc);
                error_code = UDI_ERROR_LIBRARY;
                break;
            }
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

/**
 * Submits a request where a no-op response is expected
 *
 * @param proc          the process handle
 * @param req           the request
 * @param desc          a description of the request
 * @param file          the file of the call to this function
 * @param line          the line of the call to this function
 *
 * @return the result of the operation
 */
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

/**
 * Creates a breakpoint in the specified process at the specified
 * virtual address
 *
 * @param proc          the process handle
 * @param addr          the address to place the breakpoint
 * 
 * @return the result of the operation
 */
udi_error_e create_breakpoint(udi_process *proc, udi_address addr) {
    udi_request request = create_request_breakpoint_create(addr);

    return submit_request_noresp(proc, &request, "breakpoint create request",
            __FILE__, __LINE__);
}

/**
 * Sends a breakpoint request to the specified process
 *
 * @param proc          the process handle
 * @param addr          the address of the breakpoint
 * @param request_type  the request type
 * @param desc          a description of the request
 * @param file          the file of the call to this function
 * @param line          the line of the call to this function
 *
 * @return the result of the operation
 */      
static
udi_error_e breakpoint_request(udi_process *proc, udi_address addr,
        udi_request_type request_type,
        const char *desc, const char *file, int line)
{
    udi_request request = create_request_breakpoint(request_type, addr);

    return submit_request_noresp(proc, &request, desc, file, line);
}

/**
 *
 * Install a previously created breakpoint into the specified process'
 * memory
 *
 * @param proc          the process handle
 * @param addr          the address of the breakpoint
 *
 * @return the result of the operation
 */
udi_error_e install_breakpoint(udi_process *proc, udi_address addr) {
    return breakpoint_request(proc, addr, UDI_REQ_INSTALL_BREAKPOINT,
            "breakpoint install request", __FILE__, __LINE__);
}

/**
 *
 * Remove a previously installed breakpoint from the specified process'
 * memory
 *
 * @param proc          the process handle
 * @param addr          the address of the breakpoint
 *
 * @return the result of the operation
 */
udi_error_e remove_breakpoint(udi_process *proc, udi_address addr) {
    return breakpoint_request(proc, addr, UDI_REQ_REMOVE_BREAKPOINT,
            "breakpoint remove request", __FILE__, __LINE__);

}

/**
 *
 * Delete a previously created breakpoint for the specified process
 *
 * @param proc          the process handle
 * @param addr          the address of the breakpoint
 *
 * @return the result of the operation
 */
udi_error_e delete_breakpoint(udi_process *proc, udi_address addr) {
    return breakpoint_request(proc, addr, UDI_REQ_DELETE_BREAKPOINT,
            "breakpoint delete request", __FILE__, __LINE__);
}

/**
 * Access memory in a process
 *
 * @param proc          the process handle
 * @param write         if non-zero, write to specified address
 * @param value         pointer to the value to read/write
 * @param size          the size of the data block pointed to by value
 * @param addr          the location in memory to read/write
 *
 * @return the result of the operation
 */
udi_error_e mem_access(udi_process *proc, int write, void *value, udi_length size, 
        udi_address addr) 
{
    udi_error_e error_code = UDI_ERROR_NONE;
    udi_request request = write ?
        create_request_write(addr, size, value) : 
        create_request_read(addr, size);
    
    udi_response *resp = NULL;
    do{
        error_code = submit_request(proc, &request, &resp, "memory access request",
                __FILE__, __LINE__);

        if ( error_code != UDI_ERROR_NONE ) return error_code;

        if ( resp->request_type == UDI_REQ_READ_MEM ) {
            void *result;
            udi_length result_size;
            if (unpack_response_read(resp, &result_size, &result)) {
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

/**
 * Continue a stopped UDI process
 *
 * @param proc          the process handle
 *
 * @return the result of the operation
 */
udi_error_e continue_process(udi_process *proc) {
    udi_request req = create_request_continue(0);

    return submit_request_noresp(proc, &req, "continue request",
            __FILE__, __LINE__);
}

/**
 * Refreshes the state of the specified process
 *
 * @param proc          the process handle
 *
 * @return the result of the operation
 */
udi_error_e refresh_state(udi_process *proc) {
    udi_request req = create_request_state();

    udi_response *resp = NULL;

    udi_error_e sub_result = submit_request(proc, &req, &resp, "state request",
            __FILE__, __LINE__);

    if ( sub_result == UDI_ERROR_NONE ) {
        int num_threads = 0;
        thread_state *states = NULL;
        if ( unpack_response_state(resp, &num_threads, &states) == 0) {
            int i;
            for (i = 0; i < num_threads; ++i) {
                udi_thread *thr = find_thread(proc, states[i].tid);
                if ( thr == NULL ) {
                    sub_result = UDI_ERROR_LIBRARY;
                    break;
                }
                thr->state = states[i].state;
            }
        }else{
            sub_result = UDI_ERROR_LIBRARY;
        }
            
    }

    free_response(resp);

    return sub_result;
}

/**
 * Sets the global error message and logs it
 *
 * @param resp          the response containing the error message
 * @param error_file    the file at which the error message was encountered
 * @param error_line    the line at which the error message was encountered
 */
void log_error_msg(udi_response *resp, const char *error_file, int error_line) {
    char *errmsg_local;
    udi_length errmsg_size;

    if (resp->response_type != UDI_RESP_ERROR ) return;

    if ( unpack_response_error(resp, &errmsg_size, &errmsg_local) ) {
        udi_printf("%s\n", "failed to unpack response for failed request");
    }else{
        udi_printf("request failed @[%s:%d]: %s\n", error_file, error_line, 
                errmsg_local);
        strncpy(errmsg, errmsg_local, ERRMSG_SIZE);
    }

    if ( errmsg_local != NULL ) free(errmsg_local);
}

/**
 * Frees an exported event
 *
 * @param event the exported event
 */
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

/**
 * Frees a event list returned by wait_for_events
 *
 * @param event_list the event list to free
 */
void free_event_list(udi_event *event_list) {
    udi_event *current_event = event_list;
    while ( current_event != NULL ) {
        udi_event *next_event = current_event->next_event;
        free_event(current_event);
        current_event = next_event;
    }
}

/**
 * @return a string representation of the specified event type
 */
const char *get_event_type_str(udi_event_type event_type) {
    return event_type_str(event_type);
}

/**
 * Decodes a udi_event_internal event for the specified process
 * into an exportable udi_event
 *
 * @param proc          process handle
 * @param event         the internal event
 *
 * @return the new event
 */
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

    // get the thread on which the event occurred
    switch(ret_event->event_type) {
        case UDI_EVENT_THREAD_CREATE:
        {
            udi_thread *tmp = handle_thread_create(proc, event->thread_id);
            if (tmp == NULL) {
                udi_printf("failed to create handle for thread 0x%"PRIx64"\n",
                        event->thread_id);
                free_event(ret_event);
                return NULL;
            }

            ret_event->thr = tmp;
            udi_printf("thread 0x%"PRIx64" created\n", event->thread_id);
            break;
        }
        default:
        {
            udi_thread *tmp = find_thread(proc, event->thread_id);
            if ( tmp == NULL ) {
                udi_printf("failed to find thread handle for thread 0x%"PRIx64"\n",
                        event->thread_id);
                free_event(ret_event);
                return NULL;
            }
            ret_event->thr = tmp;
            break;
        }
    }

    // event specific processing before passing the event to the user
    switch(ret_event->event_type) {
        case UDI_EVENT_THREAD_DEATH:
            if ( handle_thread_death(proc, ret_event->thr) != 0 ) {
                udi_printf("failed to complete platform-specific processing of thread death for "
                        " thread 0x%"PRIx64, ret_event->thr->tid);
                free_event(ret_event);
                return NULL;
            }
            udi_printf("thread 0x%"PRIx64" dead\n", event->thread_id);
            break;
        default:
            break;
    }


    // create event specific payload
    switch(ret_event->event_type) {
        case UDI_EVENT_THREAD_CREATE:
        case UDI_EVENT_THREAD_DEATH:
            break;
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
            if ( unpack_event_error(event, &err_event->errstr,
                        &errmsg_size) ) {
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

            if ( unpack_event_exit(event, &(exit_event->exit_code)) ) {
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

            if ( unpack_event_breakpoint(event, &(break_event->breakpoint_addr)) ) {
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
