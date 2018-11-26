/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "udirt.h"
#include "udirt-win.h"

#include "windows.h"

// constants
static const char *PIPE_NAME_BASE = "udi";

// globals
udirt_fd request_handle = INVALID_HANDLE_VALUE;
udirt_fd response_handle = INVALID_HANDLE_VALUE;
udirt_fd events_handle = INVALID_HANDLE_VALUE;

static int init_attempted = 0;
static int num_threads = 0;
static thread *threads = NULL;

void udi_abort_file_line(const char *file, unsigned int line) {
    udi_log("udi_abort at %s[%d]", file, line);

    abort();
}

int read_from(udirt_fd fd, uint8_t *dst, size_t length) {
    size_t total = 0;

    while (total < length) {
        DWORD num_read = 0;
        BOOL result = ReadFile(fd, dst + total, length - total, &num_read, NULL);
        if (!result) {
            return GetLastError();
        }

        if (num_read == 0) {
            return -1;
        }
        total += num_read;
    }

    return 0;
}

int write_to(udirt_fd fd, const uint8_t *src, size_t length) {
    size_t total = 0;
    while (total < length) {
        DWORD num_written = 0;
        BOOL result = WriteFile(fd, src + total, length - total, &num_written, NULL);
        if (!result) {
            return GetLastError();
        }

        total += num_written;
    }

    return 0;
}


void udi_log_error(format_cb cb, void *ctx, int error) {
    char buf[128];
    memset(buf, 0, 128);

    DWORD result = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL,
                                  error,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  buf,
                                  128,
                                  NULL);
    if (result == 0) {
        udi_log_string(cb, ctx, "failed to get last error string: ");
        udi_log_integer(cb, ctx, GetLastError());
    } else {
        udi_log_string(cb, ctx, buf);
    }
}

void *pre_mem_access_hook() {
    return NULL;
}

int post_mem_access_hook(void *hook_arg) {

    USE(hook_arg);

    return 0;
}

void udi_log_lock() {

}

void udi_log_unlock() {

}

udirt_fd udi_log_fd() {
    return GetStdHandle(STD_ERROR_HANDLE);
}

int get_multithread_capable() {

    return 1;
}

uint64_t get_user_thread_id() {
    return GetCurrentThreadId();
}

void rewind_pc(void *context) {
    USE(context);
}

void *get_thread_context(thread *thr) {
    USE(thr);

    return NULL;
}

int is_thread_context_valid(thread *thr) {
    USE(thr);

    return 1;
}

breakpoint *get_single_step_breakpoint(thread *thr) {
    USE(thr);

    return NULL;
}

void set_single_step_breakpoint(thread *thr, breakpoint *bp) {
    USE(thr);
    USE(bp);
}

void set_single_step(thread *thr, int single_step) {
    USE(thr);
    USE(single_step);
}

int is_event_breakpoint(breakpoint *bp) {
    USE(bp);

    return 0;
}

int handle_event_breakpoint(breakpoint *bp,
                            const void *context,
                            udi_errmsg *errmsg)
{
    USE(bp);
    USE(context);
    USE(errmsg);

    return 0;
}

thread *get_thread_list() {
    return NULL;
}

udi_thread_state_e get_thread_state(thread *thr) {
    USE(thr);

    return UDI_TS_RUNNING;
}

thread *get_next_thread(thread *thr) {
    USE(thr);

    return NULL;
}

int get_num_threads() {
    return num_threads;
}

int is_thread_dead(thread *thr) {
    USE(thr);

    return 1;
}

int thread_death_handshake(thread *thr, udi_errmsg *errmsg) {
    USE(thr);
    USE(errmsg);

    return 0;
}

void post_continue_hook(uint32_t sig_val) {
    USE(sig_val);

}

int is_single_step(thread *thr) {
    USE(thr);

    return 0;
}

uint64_t get_thread_id(thread *thr) {
    USE(thr);

    return 0;
}

const char *get_mem_errstr() {

    return NULL;
}

int get_register(udi_register_e reg,
                 udi_errmsg *errmsg,
                 uint64_t *value,
                 const void *context)
{
    USE(reg);
    USE(errmsg);
    USE(value);
    USE(context);

    return 0;
}

int set_register(udi_register_e reg,
                 udi_errmsg *errmsg,
                 uint64_t value,
                 void *context)
{
    USE(reg);
    USE(errmsg);
    USE(value);
    USE(context);

    return 0;
}

void set_thread_state(thread *thr, udi_thread_state_e state) {
    USE(thr);
    USE(state);
}

uint64_t get_pc(const void *context) {
    USE(context);

    return 0;
}

static
thread *create_thread_struct(uint64_t tid) {

    thread *new_thr = (thread *)udi_malloc(sizeof(thread));
    if (new_thr == NULL) {
        return NULL;
    }
    memset(new_thr, 0, sizeof(thread));

    new_thr->id = tid;
    new_thr->request_handle = INVALID_HANDLE_VALUE;
    new_thr->response_handle = INVALID_HANDLE_VALUE;
    new_thr->next_thread = NULL;
    new_thr->ts = UDI_TS_RUNNING;

    return new_thr;
}

static
void link_thread(thread *new_thr) {

    // find the end of the list
    thread *cur_thread = threads;
    thread *last_thread = threads;

    while (cur_thread != NULL) {
        last_thread = cur_thread;
        cur_thread = cur_thread->next_thread;
    }

    num_threads++;

    if (last_thread == NULL) {
        threads = new_thr;
    }else{
        last_thread->next_thread = new_thr;
    }

    // TODO global data updated, issue full memory barrier
}

static
thread *create_thread(uint64_t tid) {

    thread *new_thr = create_thread_struct(tid);
    if (new_thr == NULL) {
        return NULL;
    }

    link_thread(new_thr);

    return new_thr;
}

static
int wait_and_execute_command(udi_errmsg *errmsg, thread **thr) {
    int result = RESULT_ERROR;

    USE(errmsg);
    USE(thr);
    do {

    }while(0);

    return result;
}

static
LONG WINAPI exception_entry(EXCEPTION_POINTERS *events) {

    USE(events);
    // TODO generate events
    return EXCEPTION_CONTINUE_SEARCH;
}

static int create_udi_pipe(const char *pipe_name_suffix,
                           int input,
                           udi_errmsg *errmsg,
                           udirt_fd *output) {
    char pipe_name[128];
    memset(pipe_name, 0, 128);

    udi_formatted_str(pipe_name, 128, "\\\\.\\pipe\\%s-%d-%s",
                      PIPE_NAME_BASE,
                      GetCurrentProcessId(),
                      pipe_name_suffix);
    udi_log("creating pipe %s", pipe_name);

    udirt_fd pipe = CreateNamedPipeA(pipe_name,
                                     input ? PIPE_ACCESS_INBOUND : PIPE_ACCESS_OUTBOUND,
                                     PIPE_TYPE_BYTE |
                                     PIPE_READMODE_BYTE |
                                     PIPE_WAIT |
                                     PIPE_REJECT_REMOTE_CLIENTS,
                                     PIPE_UNLIMITED_INSTANCES,
                                     1024,
                                     1024,
                                     0,
                                     NULL);
    if (pipe == INVALID_HANDLE_VALUE) {
        udi_set_errmsg(errmsg, "error creating %s pipe: %e", pipe_name_suffix, GetLastError());
        return RESULT_ERROR;
    }

    *output = pipe;
    return RESULT_SUCCESS;
}

static int connect_pipe(const char *pipe_name_suffix, udirt_fd handle, udi_errmsg *errmsg) {

    if (!ConnectNamedPipe(handle, NULL)) {
        int last_error = GetLastError();
        if (last_error != ERROR_PIPE_CONNECTED) {
            udi_set_errmsg(errmsg, "failed to connect %s pipe: %e", pipe_name_suffix, last_error);
            return RESULT_ERROR;
        }
    }

    return RESULT_SUCCESS;
}

static
thread *create_initial_thread() {
    return create_thread(get_user_thread_id());
}

static
int thread_create_response_fd_callback(void *ctx, udirt_fd *resp_fd, udi_errmsg *errmsg) {

    thread *thr = (thread *)ctx;
    char thr_resp_pipe_name[32];

    udi_formatted_str(thr_resp_pipe_name, 32, "%x-%s", thr->id, RESPONSE_FILE_NAME);

    int result = connect_pipe(thr_resp_pipe_name, thr->response_handle, errmsg);
    if ( result != RESULT_SUCCESS ) {
        return result;
    }

    *resp_fd = thr->response_handle;
    return RESULT_SUCCESS;
}

static
int thread_create_handshake(thread *thr, udi_errmsg *errmsg) {

    char thr_req_pipe_name[32];
    udi_formatted_str(thr_req_pipe_name, 32, "%x-%s", thr->id, REQUEST_FILE_NAME);

    int result = connect_pipe(thr_req_pipe_name, thr->request_handle, errmsg);
    if (result != RESULT_SUCCESS) {
        return result;
    }

    result = perform_init_handshake(thr->request_handle,
                                    thread_create_response_fd_callback,
                                    thr,
                                    thr->id,
                                    errmsg);
    if (result != RESULT_SUCCESS) {
        udi_log("failed to complete init handshake for thread %a", thr->id);
        return result;
    }

    return RESULT_SUCCESS;
}

static
int thread_create_callback(thread *thr, udi_errmsg *errmsg) {

    char thr_req_pipe_name[32];
    char thr_resp_pipe_name[32];

    udi_formatted_str(thr_req_pipe_name, 32, "%x-%s", thr->id, REQUEST_FILE_NAME);
    udi_formatted_str(thr_resp_pipe_name, 32, "%x-%s", thr->id, RESPONSE_FILE_NAME);

    int result = create_udi_pipe(thr_req_pipe_name, 1, errmsg, &(thr->request_handle));
    if (result != RESULT_SUCCESS) {
        return result;
    }

    result = create_udi_pipe(thr_resp_pipe_name, 0, errmsg, &(thr->response_handle));
    if (result != RESULT_SUCCESS) {
        return result;
    }

    return RESULT_SUCCESS;
}

static
int process_response_fd_callback(void *ctx, udirt_fd *resp_fd, udi_errmsg *errmsg) {

    thread **output = (thread **)ctx;

    int result = connect_pipe(RESPONSE_FILE_NAME, response_handle, errmsg);
    if (result != RESULT_SUCCESS) {
        return result;
    }

    thread *thr = create_initial_thread();
    if (thr == NULL) {
        udi_set_errmsg(errmsg, "failed to create initial thread");
        return RESULT_ERROR;
    }

    result = thread_create_callback(thr, errmsg);
    if (result != RESULT_SUCCESS) {
        udi_log("failed to create thread-specific files: %s", errmsg->msg);
        return result;
    }

    result = connect_pipe(EVENTS_FILE_NAME, events_handle, errmsg);
    if (result != RESULT_SUCCESS) {
        return result;
    }

    *output = thr;
    *resp_fd = response_handle;
    return RESULT_SUCCESS;
}

static
int handshake_with_debugger(udi_errmsg *errmsg) {
    int result = RESULT_SUCCESS;
    do {

        result = connect_pipe(REQUEST_FILE_NAME, request_handle, errmsg);
        if (result != RESULT_SUCCESS) {
            break;
        }

        thread *thr = NULL;
        result = perform_init_handshake(request_handle,
                                        process_response_fd_callback,
                                        &thr,
                                        get_user_thread_id(),
                                        errmsg);
        if (result != RESULT_SUCCESS) {
            break;
        }

        result = thread_create_handshake(thr, errmsg);
        if (result != RESULT_SUCCESS) {
            udi_log("failed to complete thread create handshake: %s", errmsg->msg);
            break;
        }


        // after sending init response, any debugging operations are valid
        udi_enabled = 1;
    }while(0);

    return result;
}

static
int create_udi_filesystem(udi_errmsg *errmsg) {

    int result = create_udi_pipe(REQUEST_FILE_NAME, 1, errmsg, &request_handle);
    if (result != RESULT_SUCCESS) {
        return result;
    }

    result = create_udi_pipe(RESPONSE_FILE_NAME, 0, errmsg, &response_handle);
    if (result != RESULT_SUCCESS) {
        return result;
    }

    result = create_udi_pipe(EVENTS_FILE_NAME, 0, errmsg, &events_handle);
    if (result != RESULT_SUCCESS) {
        return result;
    }

    return RESULT_SUCCESS;
}

static
void enable_debug_logging() {
    // turn on debugging, if necessary
    char debug_env_value[8];
    size_t length = 8;

    if (!getenv_s(&length, debug_env_value, length, UDI_DEBUG_ENV)) {
        udi_debug_on = 1;
    }
}

static
void init_udirt() {
    udi_errmsg errmsg;
    errmsg.size = ERRMSG_SIZE;
    errmsg.msg[ERRMSG_SIZE-1] = '\0';

    enable_debug_logging();

    int result;
    do {
        void *handle = AddVectoredExceptionHandler(1, exception_entry);
        if (handle == NULL) {
            udi_set_errmsg(&errmsg,
                           "failed to set exception handler");
            result = RESULT_ERROR;
            break;
        }

        result = create_udi_filesystem(&errmsg);
        if ( result != RESULT_SUCCESS ) {
            udi_log("failed to create udi filesystem");
            break;
        }

        result = handshake_with_debugger(&errmsg);
        if (result != RESULT_SUCCESS) {
            udi_log("failed to complete handshake with debugger");
            break;
        }

        thread *request_thr = NULL;
        result = wait_and_execute_command(&errmsg, &request_thr);
        if ( result != RESULT_SUCCESS ) {
            udi_log("failed to handle initial command");
            break;
        }
    }while(0);

    if (result != RESULT_SUCCESS ) {
        udi_enabled = 0;

        udi_log("initialization failed: %s", errmsg.msg);
        udi_abort();
    }else{
        udi_log("initialization completed");
    }
}

BOOL WINAPI DllMain(HINSTANCE *dll, DWORD reason, LPVOID reserved) {
    USE(dll);
    USE(reserved);

    HANDLE thread = GetCurrentThread();
    DWORD tid = GetThreadId(thread);

    switch (reason) {
        case DLL_PROCESS_ATTACH:
            fprintf(stderr, "library attached (tid = %08lx)\n", tid);
            break;
        case DLL_PROCESS_DETACH:
            fprintf(stderr, "library detached (tid = %08lx)\n", tid);
            break;
        case DLL_THREAD_ATTACH:
            fprintf(stderr, "thread attached (tid = %08lx)\n", tid);
            if ( !init_attempted ) {
                init_udirt();
                init_attempted = 1;
            }
            break;
        case DLL_THREAD_DETACH:
            fprintf(stderr, "thread detached (tid = %08lx)\n", tid);
            break;
        default:
            break;
    }

    return TRUE;
}
