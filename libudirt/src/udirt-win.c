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
static const unsigned int PID_STR_LEN = 10;

// globals
udirt_fd request_handle = INVALID_HANDLE_VALUE;
udirt_fd response_handle = INVALID_HANDLE_VALUE;
udirt_fd events_handle = INVALID_HANDLE_VALUE;

void udi_abort_file_line(const char *file, unsigned int line) {
    udi_log("udi_abort at %s[%d]", file, line);

    abort();
}

int read_from(udirt_fd fd, uint8_t *dst, size_t length) {
    size_t total = 0;

    while (total < length) {
        size_t num_read = 0;
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
        size_t num_written = 0;
        BOOL result = WriteFile(fd, src + total, length - total, &num_written, NULL);
        if (!result) {
            return GetLastError();
        }

        total += num_written;
    }

    return 0;
}


void udi_log_error(format_cb cb, void *ctx, int error) {
    char buf[64];
    memset(buf, 0, 64);

    DWORD result = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL,
                                  error,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  &buf,
                                  64,
                                  NULL);
    if (result == 0) {
        udi_log_string(cb, ctx, "failed to get last error string");
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
    return 1;
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
int wait_and_execute_command(udi_errmsg *errmsg, thread **thr) {
    int result = RESULT_ERROR;
    do {

    }while(0);

    return result;
}

static
LONG WINAPI exception_entry(EXCEPTION_POINTERS *exception_info) {
    // TODO generate events
    return EXCEPTION_CONTINUE_SEARCH;
}

static int create_udi_pipe(const char *pipe_name_suffix,
                           int input,
                           udirt_fd **output) {
    char pipe_name[64];
    memset(pipe_name, 0, 64);

    udi_formatted_str(pipe_name, 64, "\\.\\pipe\\%s-%d-%s",
                      PIPE_NAME_BASE,
                      GetCurrentProcessId(),
                      pipe_name_suffix);

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
        udi_log("error creating %s pipe: %e", pipe_name_suffix, GetLastError());
        return 1;
    }

    *output = pipe;
    return 0;
}

static int create_udi_filesystem() {
    int result;

    do {
        result = create_udi_pipe(REQUEST_FILE_NAME, 1, &request_handle);
        if ( result != 0) {
            break;
        }

        result = create_udi_pipe(RESPONSE_FILE_NAME, 0, &response_handle);
        if ( result != 0) {
            break;
        }

        result = create_udi_pipe(EVENTS_FILE_NAME, 0, &events_handle);
        if ( result != 0) {
            break;
        }
    }while(0);

    return result;
}

static
int handshake_with_debugger(udi_errmsg *errmsg) {
    int result = RESULT_ERROR;
    do {

    }while(0);

    return result;
}

static
void init_udirt() {
    udi_errmsg errmsg;
    errmsg.size = ERRMSG_SIZE;
    errmsg.msg[ERRMSG_SIZE-1] = '\0';

    int result;
    do {
        void *handle = AddVectoredExceptionHandler(1, exception_entry);
        if (handle == NULL) {
            udi_set_errmsg(&errmsg,
                           "failed to set exception handler");
            result = RESULT_ERROR;
            break;
        }

        result = create_udi_filesystem();
        if ( result != 0) {
            udi_set_errmsg(&errmsg,
                           "failed to create udi filesystem");
            result = RESULT_ERROR;
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
            break;
        case DLL_THREAD_DETACH:
            fprintf(stderr, "thread detached (tid = %08lx)\n", tid);
            break;
        default:
            break;
    }

    return TRUE;
}
