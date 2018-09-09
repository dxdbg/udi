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

#include "udirt.h"

#include "windows.h"

const char * const DEFAULT_UDI_ROOT_DIR;
const char * const UDI_DS;
const unsigned int DS_LEN;

char *UDI_ROOT_DIR;

// constants
const char * const UDI_DS = "/";
const unsigned int DS_LEN = 1;
const char * const DEFAULT_UDI_ROOT_DIR = "C:\\tmp\\udi";

// globals
udirt_fd events_handle = INVALID_HANDLE_VALUE;

void udi_abort_file_line(const char *file, unsigned int line) {
    udi_log("udi_abort at %s[%d]", file, line);

    abort();
}

int read_from(udirt_fd fd, uint8_t *dst, size_t length) {
    USE(fd);
    USE(dst);
    USE(length);

    return -1;
}

int write_to(udirt_fd fd, const uint8_t *src, size_t length) {
    USE(fd);
    USE(src);
    USE(length);

    return -1;
}


void udi_log_error(format_cb cb, void *ctx, int error) {
    USE(cb);
    USE(ctx);
    USE(error);
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

    return INVALID_HANDLE_VALUE;
}

int get_multithread_capable() {

    return 0;
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

void set_thread_state(thread *thr, udi_thread_state_e state)
{
    USE(thr);
    USE(state);
}

uint64_t get_pc(const void *context)
{
    USE(context);

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE *dll, DWORD reason, LPVOID reserved)
{
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
