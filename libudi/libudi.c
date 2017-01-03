/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

///////////////////////
// Library functions //
///////////////////////

const char *get_last_error_message(udi_process *proc) {
    switch(proc->error_code) {
        case UDI_ERROR_LIBRARY:
            return "UDI library internal error";
        case UDI_ERROR_REQUEST:
            return proc->errmsg.msg;
        default:
            return "Error not set";
    }
}

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
 * Sets the directory to be used for the root of the UDI filesystem for the specified process
 * 
 * @param proc the process
 * @param root_dir the directory to set as the root
 *
 * @return non-zero on failure
 * @return zero on success
 */
static
int set_root_dir(udi_process *proc, const char *root_dir) {
    size_t length = strlen(root_dir);

    char *tmp_root_dir = (char *)malloc(length+1);
    if (tmp_root_dir == NULL) {
        return -1;
    }

    strncpy((char *)tmp_root_dir, root_dir, length);
    tmp_root_dir[length] = '\0';

    proc->root_dir = tmp_root_dir;

    return 0;
}

/**
 * Initializes the library
 */
LIB_INIT_FUNC(init_libudi);
void init_libudi() {
    check_debug_logging();
}

/**
 * Allocates the error for the create process function
 *
 * @param code          the actual error code
 * @param errmsg        the actual error message
 * @param dest_code     the destination code
 * @param dest_errmsg   the error message
 * @param errmsg_size   the error message size
 */
static
void allocate_error(udi_error_e code, 
        const char *errmsg,
        udi_error_e *dest_code, 
        char **dest_errmsg)
{
    udi_printf("%s\n", errmsg);

    *dest_code = code;
    size_t errmsg_size = strlen(errmsg) + 1;

    char *local_errmsg = (char *)malloc(errmsg_size);
    strncpy(local_errmsg, errmsg, errmsg_size-1);
    local_errmsg[errmsg_size-1] = '\0';

    *dest_errmsg = local_errmsg;
}

udi_process *create_process(const char *executable, char * const argv[],
        char * const envp[], const udi_proc_config *config,
        udi_error_e *error_code, char **errmsg)
{
    // Validate arguments
    if (argv == NULL || executable == NULL) {
        allocate_error(UDI_ERROR_REQUEST, "invalid arguments", error_code, errmsg);
        return NULL;
    }

    udi_error_e local_error = UDI_ERROR_NONE;
    char *local_errmsg = NULL;
    udi_process *proc = (udi_process *)malloc(sizeof(udi_process));
    do{
        if ( proc == NULL ) {
            local_errmsg = "malloc failed";
            local_error = UDI_ERROR_LIBRARY;
            break;
        }

        memset(proc, 0, sizeof(udi_process));
        proc->error_code = UDI_ERROR_NONE;
        proc->errmsg.size = ERRMSG_SIZE;
        proc->errmsg.msg[ERRMSG_SIZE-1] = '\0';
        proc->running = 0;
        proc->terminated = 0;
        proc->terminating = 0;

        if ( config->root_dir != NULL ) {
            if ( set_root_dir(proc, config->root_dir) != 0 ) {
                local_errmsg = "failed to set root directory";
                local_error = UDI_ERROR_LIBRARY;
                break;
            }
        }else{
            proc->root_dir = DEFAULT_UDI_ROOT_DIR;
        }

        proc->pid = fork_process(proc, executable, argv, envp);
        if ( proc->pid == INVALID_UDI_PID ) {
            local_errmsg = "failed to create process";
            local_error = UDI_ERROR_REQUEST;
            break;
        }

        if ( initialize_process(proc) != 0 ) {
            local_errmsg = "failed to initialize process";
            local_error = UDI_ERROR_LIBRARY;
            break;
        }
    }while(0);

    *error_code = local_error;
    if ( local_errmsg != NULL ) {
        if ( proc ) {
            free(proc);
            proc = NULL;
        }

        allocate_error(local_error, local_errmsg, error_code, errmsg);
    }

    return proc;
}

udi_error_e free_process(udi_process *proc) {
    // TODO detach from the process, etc.
    
    if ( proc->root_dir != DEFAULT_UDI_ROOT_DIR ) {
        free((void *)proc->root_dir);
    }

    free(proc);

    return UDI_ERROR_NONE;
}

void set_user_data(udi_process *proc, void *user_data) {
    proc->user_data = user_data;
}

void *get_user_data(udi_process *proc) {
    return proc->user_data;
}

void set_thread_user_data(udi_thread *thr, void *user_data) {
    thr->user_data = user_data;
}

void *get_thread_user_data(udi_thread *thr) {
    return thr->user_data;
}

int get_proc_pid(udi_process *proc) {
    return proc->pid;
}

udi_arch_e get_proc_architecture(udi_process *proc) {
    return proc->architecture;
}

int get_multithread_capable(udi_process *proc) {
    return proc->multithread_capable;
}

uint64_t get_tid(udi_thread *thr) {
    return thr->tid;
}

udi_process *get_process(udi_thread *thr) {
    return thr->proc;
}

udi_thread_state_e get_state(udi_thread *thr) {
    return thr->state;
}

udi_thread *get_initial_thread(udi_process *proc) {
    udi_thread *iter = proc->threads;

    while (iter != NULL) {
        if (iter->initial) break;

        iter = iter->next_thread;
    }

    return iter;
}

int is_running(udi_process *proc) {
    return proc->running;
}

int is_terminated(udi_process *proc) {
    return proc->terminated;
}

udi_thread *get_next_thread(udi_thread *thr) {
    return thr->next_thread;
}

/**
 * @return non-zero if the specified request_type has request contenta
 */
static
int has_request_content(udi_request_type_e request_type) {
    switch (request_type) {
        case UDI_REQ_STATE:
            return 0;
        default:
            break;
    }

    return 1;
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
        if (proc->running) {
            error_code = UDI_ERROR_REQUEST;
            snprintf(proc->errmsg.msg, ERRMSG_SIZE-1, "process[%d] is running, cannot send request",
                    proc->pid);
            break;
        }

        int valid_request_type = 1;
        switch (req->request_type) {
            case UDI_REQ_THREAD_SUSPEND:
            case UDI_REQ_THREAD_RESUME:
                error_code = UDI_ERROR_REQUEST;
                snprintf(proc->errmsg.msg, ERRMSG_SIZE-1, "request invalid for process");
                udi_printf("%s\n", proc->errmsg.msg);
                valid_request_type = 0;
                break;
            default:
                break;
        }
        if ( !valid_request_type ) break;

        if ( has_request_content(req->request_type) ) {
            if ( req->packed_data == NULL ) {
                udi_printf("failed to pack data for %s\n", desc);
                error_code = UDI_ERROR_LIBRARY;
                break;
            }
        }

        int resp_expected = 1;
        // No response is expected when continuing a terminating process
        if (req->request_type == UDI_REQ_CONTINUE && proc->terminating) {
            resp_expected = 0;
        }

        if (req->request_type == UDI_REQ_CONTINUE) {
            proc->running = 1;
        }

        // Perform the request
        if ( write_request(req, proc) != 0 ) {
            udi_printf("failed to write %s\n", desc);
            error_code = UDI_ERROR_LIBRARY;
            break;
        }

        if (!resp_expected) {
            error_code = UDI_ERROR_NONE;
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
            log_error_msg(proc, *resp, file, line);
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

    proc->error_code = error_code;
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
 * Submits the specified request to the specified process
 *
 * @param thr           the thread
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
udi_error_e submit_request_thr(udi_thread *thr, udi_request *req, udi_response **resp,
        const char *desc, const char *file, int line)
{

    udi_error_e error_code = UDI_ERROR_NONE;

    do {
        // Perform the request
        if ( write_request_thr(req, thr) != 0 ) {
            udi_printf("failed to write %s\n", desc);
            error_code = UDI_ERROR_LIBRARY;
            break;
        }

        // Get the response
        *resp = read_response_thr(thr);

        if ( *resp == NULL ) {
            udi_printf("failed to read response for %s\n", desc);
            error_code = UDI_ERROR_LIBRARY;
            break;
        }

        if ( (*resp)->response_type == UDI_RESP_ERROR ) {
            log_error_msg(get_process(thr), *resp, file, line);
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

    thr->proc->error_code = error_code;
    return error_code;

}

/**
 * Submits a request where a no-op response is expected
 *
 * @param thr           the thread
 * @param req           the request
 * @param desc          a description of the request
 * @param file          the file of the call to this function
 * @param line          the line of the call to this function
 *
 * @return the result of the operation
 */
static
udi_error_e submit_request_thr_noresp(udi_thread *thr,
        udi_request *req, const char *desc, const char *file,
        int line)
{
    udi_response *resp = NULL;
    udi_error_e error_code = submit_request_thr(thr, req, &resp, desc,
            file, line);

    if ( resp != NULL ) free_response(resp);

    return error_code;
}

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

udi_error_e register_access(udi_thread *thr, int write, udi_register_e reg, udi_address *value) {
    udi_error_e error_code = UDI_ERROR_NONE;
    udi_request request = write ?
        create_request_write_reg(reg, *value) :
        create_request_read_reg(reg);

    udi_response *resp = NULL;
    do {
        error_code = submit_request_thr(thr, &request, &resp, "register access request",
                __FILE__, __LINE__);

        if (error_code != UDI_ERROR_NONE) break;

        if ( resp->request_type == UDI_REQ_READ_REGISTER ) {
            if (unpack_response_read_register(resp, value)) {
                snprintf(thr->proc->errmsg.msg,
                         thr->proc->errmsg.size,
                         "%s",
                         "failed to unpack response for read register request");
                udi_printf("%s\n", thr->proc->errmsg.msg);
                error_code = UDI_ERROR_LIBRARY;
                break;
            }
        }
    }while(0);

    free_response(resp);

    return error_code;
}

udi_error_e get_pc(udi_thread *thr, udi_address *pc) {
    udi_arch_e arch = get_proc_architecture(thr->proc);
    udi_register_e reg;
    switch (arch) {
        case UDI_ARCH_X86:
            reg = UDI_X86_EIP;
            break;
        case UDI_ARCH_X86_64:
            reg = UDI_X86_64_RIP;
            break;
        default:
            udi_printf("Unsupported architecture %d\n", arch);
            return UDI_ERROR_LIBRARY;
    }

    return register_access(thr, 0, reg, pc);
}

udi_error_e get_next_instruction(udi_thread *thr, udi_address *instr) {

    udi_error_e error_code = UDI_ERROR_NONE;
    udi_request req = create_request_next_instr();

    udi_response *resp = NULL;
    do {
        error_code = submit_request_thr(thr, &req, &resp, "next instruction", __FILE__,
                __LINE__);

        if (error_code != UDI_ERROR_NONE) break;

        if ( unpack_response_next_instr(resp, instr) ) {
            snprintf(thr->proc->errmsg.msg,
                     thr->proc->errmsg.size,
                     "%s",
                     "failed to unpack response to next instruction request");
            udi_printf("%s\n", thr->proc->errmsg.msg);
            error_code = UDI_ERROR_LIBRARY;
        }
    }while(0);

    free_response(resp);

    return error_code;
}

udi_error_e continue_process(udi_process *proc) {
    if (proc->running) {
        snprintf(proc->errmsg.msg, proc->errmsg.size, "process[%d] is already running",
                proc->pid);
        udi_printf("%s\n", proc->errmsg.msg);
        return UDI_ERROR_REQUEST;
    }

    udi_request req = create_request_continue(0);

    udi_error_e result = submit_request_noresp(proc, &req, "continue request",
            __FILE__, __LINE__);
    
    if (result != UDI_ERROR_NONE) {
        proc->running = 0;
    }

    return result;
}

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

udi_error_e resume_thread(udi_thread *thr) {
    udi_request req = create_request_thr_state(UDI_TS_RUNNING);

    return submit_request_thr_noresp(thr, &req, "thread resume", __FILE__, __LINE__);
}

udi_error_e suspend_thread(udi_thread *thr) {
    udi_request req = create_request_thr_state(UDI_TS_SUSPENDED);

    return submit_request_thr_noresp(thr, &req, "thread suspend", __FILE__, __LINE__);
}

udi_error_e set_single_step(udi_thread *thr, int enable) {

    udi_error_e error_code = UDI_ERROR_NONE;
    udi_request req = create_request_single_step(enable);

    udi_response *resp = NULL;
    do {
        error_code = submit_request_thr(thr, &req, &resp, "single step modification",
                __FILE__, __LINE__);

        if (error_code != UDI_ERROR_NONE) break;

        // Cache the setting
        thr->single_step = enable;
    }while(0);

    free_response(resp);

    return error_code;
}

int get_single_step(udi_thread *thr) {
    return thr->single_step;
}

/**
 * Sets the process error message and logs it
 *
 * @param proc          the process
 * @param resp          the response containing the error message
 * @param error_file    the file at which the error message was encountered
 * @param error_line    the line at which the error message was encountered
 */
void log_error_msg(udi_process *proc, udi_response *resp, const char *error_file, int error_line) {
    char *errmsg_local;

    if (resp->response_type != UDI_RESP_ERROR ) return;

    if ( unpack_response_error(resp, &(proc->errmsg.size), &errmsg_local) ) {
        udi_printf("%s\n", "failed to unpack response for failed request");
    }else{
        udi_printf("request failed @[%s:%d]: %s\n", error_file, error_line, 
                errmsg_local);
        strncpy(proc->errmsg.msg, errmsg_local, proc->errmsg.size);
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

void free_event_list(udi_event *event_list) {
    if (event_list == NULL) return;

    udi_event *current_event = event_list;
    while ( current_event != NULL ) {
        udi_event *next_event = current_event->next_event;
        free_event(current_event);
        current_event = next_event;
    }
}

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
    udi_thread *tmp = find_thread(proc, event->thread_id);
    if ( tmp == NULL ) {
        udi_printf("failed to find thread handle for thread 0x%"PRIx64"\n",
                event->thread_id);
        free_event(ret_event);
        return NULL;
    }
    ret_event->thr = tmp;

    // create event specific payload
    switch(ret_event->event_type) {
        case UDI_EVENT_THREAD_CREATE:
        {
            udi_event_thread_create *create_event = (udi_event_thread_create *)
                malloc(sizeof(udi_event_thread_create));
            if ( create_event == NULL ) {
                udi_printf("failed to malloc create event: %s\n",
                        strerror(errno));
                free_event(ret_event);
                return NULL;
            }

            uint64_t new_thr_id;
            if ( unpack_event_thread_create(event, &new_thr_id) ) {
                udi_printf("%s\n", "failed to decode thread create event");

                free(create_event);
                free(ret_event);
                return NULL;
            }

            udi_thread *new_thr = handle_thread_create(proc, new_thr_id);
            if ( new_thr == NULL ) {
                udi_printf("failed to create handle for thread0x%"PRIx64"\n",
                        new_thr_id);
                free(create_event);
                free_event(ret_event);
                return NULL;
            }
            udi_printf("thread 0x%"PRIx64" created by 0x%"PRIx64"\n", new_thr_id,
                    event->thread_id);

            create_event->new_thr = new_thr;
            ret_event->event_data = create_event;
            break;
        }
        case UDI_EVENT_THREAD_DEATH:
        {
            // No special unpacking, but need to handle the thread death 
            if ( handle_thread_death(proc, ret_event->thr) != 0 ) {
                udi_printf("failed to complete platform-specific processing of thread death for "
                        " thread 0x%"PRIx64, ret_event->thr->tid);
                free_event(ret_event);
                return NULL;
            }
            udi_printf("thread 0x%"PRIx64" dead\n", event->thread_id);
            break;
        }
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
        // The following events don't have any extra information
        case UDI_EVENT_SINGLE_STEP:
        {
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
