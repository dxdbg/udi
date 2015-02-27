/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _LIBUDI_PRIVATE_H
#define _LIBUDI_PRIVATE_H 1

#include "libudi.h"
#include "udi-common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef UNIX
typedef int udi_handle;
typedef pid_t udi_pid;
typedef uint64_t udi_tid;

#define LIB_INIT_FUNC(f) void f() __attribute__((constructor));
#else
#error Unknown platform
#endif

struct udi_thread_struct {
    int initial;
    udi_tid tid;
    udi_handle request_handle;
    udi_handle response_handle;
    udi_thread_state_e state;
    int single_step;
    void *user_data;
    struct udi_process_struct *proc;
    struct udi_thread_struct *next_thread;
};

extern const udi_pid INVALID_UDI_PID;
struct udi_process_struct {
    udi_pid pid;
    udi_handle request_handle;
    udi_handle response_handle;
    udi_handle events_handle;
    udi_arch_e architecture;
    uint32_t protocol_version;
    int multithread_capable;
    int running;
    int terminated;
    void *user_data;
    struct udi_thread_struct *threads;
    const char *root_dir;
    udi_errmsg errmsg;
    udi_error_e error_code;
};

// process handling
udi_pid fork_process(struct udi_process_struct *proc, const char *executable, char * const argv[],
        char * const envp[]);
int initialize_process(udi_process *proc);

// thread handling
udi_thread *handle_thread_create(udi_process *proc, uint64_t tid);
int handle_thread_death(udi_process *proc, udi_thread *thr);

// request handling
int write_request(udi_request *req, udi_process *proc);
int write_request_thr(udi_request *req, udi_thread *thr);
udi_response *read_response(udi_process *proc);
udi_response *read_response_thr(udi_thread *thr);
int read_event(udi_process *proc, udi_event **output);
udi_event *decode_event(udi_process *proc, udi_event_internal *event);

// logging
void log_error_msg(udi_process *proc, udi_response *resp, const char *error_file, int error_line);
void check_debug_logging();

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
