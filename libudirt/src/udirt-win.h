/*
 * Copyright (c) 2011-2023, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _UDI_RT_WIN_H
#define _UDI_RT_WIN_H 1

// This needs to be included first to set feature macros
#include "udirt-platform.h"

#include "udirt.h"

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#endif

struct udi_pipe_ctx_struct {
    OVERLAPPED ol;
    HANDLE handle;
    int pending;
};

struct thread_struct {
    uint64_t id;
    udi_thread_state_e ts;
    udi_pipe_ctx request_pipe;
    udi_pipe_ctx response_pipe;
    int single_step;
    breakpoint *single_step_bp;
    CONTEXT context;
    struct thread_struct *next_thread;
};


#ifdef __cplusplus
} // extern C
#endif

#endif /* _UDI_RT_WIN_H */
