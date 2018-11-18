/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// UDI debuggee implementation common between all platforms

#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>

#include "udirt.h"

// constants
enum { BREAKPOINT_HASH_SIZE = 256 };

const uint64_t UDI_SINGLE_THREAD_ID = 0xC0FFEEABC;

const char * const REQUEST_FILE_NAME = "request";
const char * const RESPONSE_FILE_NAME = "response";
const char * const EVENTS_FILE_NAME = "events";
const char * const UDI_DEBUG_ENV = "UDI_DEBUG";

const int RESULT_SUCCESS = 0;
const int RESULT_ERROR = -1;
const int RESULT_FAILURE = -2;

 // not enabled until initialization complete
int udi_debug_on = 0;
int udi_enabled = 0;

// read / write handling
static const uint8_t *mem_access_addr = NULL;
static size_t mem_access_size = 0;

static int aborting_mem_access = 0;
static int performing_mem_access = 0;

static void *mem_abort_label = NULL;

const uint8_t *get_mem_access_addr() {
    return mem_access_addr;
}

size_t get_mem_access_size() {
    return mem_access_size;
}

unsigned long abort_mem_access() {
    aborting_mem_access = 1;

    return (unsigned long)mem_abort_label;
}

int is_performing_mem_access() {
    return performing_mem_access;
}

/**
 * Copy memory byte to byte to allow a signal handler to abort
 * the copy
 *
 * @param dest the destination memory address
 * @param src the source memory address
 * @param n the number of bytes to copy
 *
 * @return 0 on success; non-zero otherwise
 *
 * @see memcpy
 */
static
int abortable_memcpy(uint8_t *dest, const uint8_t *src, size_t n) {
    // This should stop the compiler from messing with the label
    static void *abort_label_addr = &&abort_label;

    mem_abort_label = abort_label_addr;

    // Hopefully when compiled with optimization, this code will be vectorized if possible

    for (size_t i = 0; i < n; ++i) {
        dest[i] = src[i];
    }

abort_label:
    if ( aborting_mem_access ) {
        aborting_mem_access = 0;
        return -1;
    }

    return 0;
}

/**
 * Copy memory with hooks before and after the copy to allow
 * for example changing permissions on pages in memory.
 *
 * @param dest the destination memory address
 * @param src the source memory address
 * @param num_bytes the number of bytes
 * @param errmsg the error message populated by this method
 *
 * @return 0 on success; non-zero otherwise
 */
static
int udi_memcpy(uint8_t *dest, const uint8_t *src, size_t num_bytes, udi_errmsg *errmsg) {

    int mem_result = 0;

    void *pre_access_result = pre_mem_access_hook();

    if ( pre_access_result == NULL ) {
        udi_set_errmsg(errmsg, "pre memory access hook failed");
        return -1;
    }

    performing_mem_access = 1;
    mem_result = abortable_memcpy(dest, src, num_bytes);
    performing_mem_access = 0;

    if ( post_mem_access_hook(pre_access_result) ) {
        udi_set_errmsg(errmsg, "post memory access hook failed");
        return -1;
    }

    return mem_result;
}

/**
 * Reads memory from the specified source into the specified
 * destination
 *
 * @param dest the destination memory address
 * @param src the source memory address
 * @param num_bytes the number of bytes to read
 * @param errmsg the error message possibly populated by this function
 *
 * @return 0, success; non-zero otherwise
 */
int read_memory(uint8_t *dest, const uint8_t *src, size_t num_bytes, udi_errmsg *errmsg) {
    mem_access_addr = (void *)src;
    mem_access_size = num_bytes;

    return udi_memcpy(dest, src, num_bytes, errmsg);
}

/**
 * Writes into the specified memory address using the specified
 * source.
 *
 * @param dest the destination of the write
 * @param src the source of the write
 * @param num_bytes the number of bytes to write
 * @param errmsg the error message set by this function
 */
int write_memory(uint8_t *dest, const uint8_t *src, size_t num_bytes, udi_errmsg *errmsg) {
    mem_access_addr = dest;
    mem_access_size = num_bytes;

    return udi_memcpy(dest, src, num_bytes, errmsg);
}

// breakpoint implementation

static breakpoint *breakpoints[BREAKPOINT_HASH_SIZE];

static inline
unsigned int breakpoint_hash(uint64_t address) {
    return (unsigned int)(address % BREAKPOINT_HASH_SIZE);
}

/**
 * Creates a breakpoint at the specified address
 *
 * @param breakpoint_addr the breakpoint address
 *
 * @return the created breakpoint, NULL on failure
 */
breakpoint *create_breakpoint(uint64_t breakpoint_addr) {
    breakpoint *new_breakpoint = (breakpoint *)udi_malloc(sizeof(breakpoint));

    if ( new_breakpoint == NULL ) {
        udi_log("failed to allocate memory for breakpoint");
        return NULL;
    }

    memset(new_breakpoint, 0, sizeof(breakpoint));
    memset(new_breakpoint->saved_bytes, 0, sizeof(new_breakpoint->saved_bytes));
    new_breakpoint->address = breakpoint_addr;
    new_breakpoint->in_memory = 0;
    new_breakpoint->thread = NULL;

    unsigned int hash = breakpoint_hash(breakpoint_addr);

    // Add the breakpoint to the hash
    breakpoint *current_bp = breakpoints[hash];
    if (current_bp == NULL) {
        breakpoints[hash] = new_breakpoint;
        new_breakpoint->next_breakpoint = NULL;
    }else{
        breakpoint *tmp_breakpoint = current_bp;
        while(1) {
            if ( tmp_breakpoint->address == new_breakpoint->address ) {
                // Use the existing breakpoint
                udi_free(new_breakpoint);
                new_breakpoint = tmp_breakpoint;
                break;
            }

            if ( tmp_breakpoint->next_breakpoint == NULL ) {
                tmp_breakpoint->next_breakpoint = new_breakpoint;
                break;
            }
            tmp_breakpoint = tmp_breakpoint->next_breakpoint;
        }
    }

    return new_breakpoint;
}

/**
 * Installs the breakpoint into memory
 *
 * @param bp the breakpoint to install
 * @param errmsg the errmsg populated by the memory access
 *
 * @return 0 on success; non-zero otherwise
 */
int install_breakpoint(breakpoint *bp, udi_errmsg *errmsg) {
    if ( bp->in_memory ) return 0;

    int result = write_breakpoint_instruction(bp, errmsg);

    if ( result == 0 ) {
        bp->in_memory = 1;
    }

    return result;
}

/**
 * Removes the breakpoint from memory
 *
 * @param bp the breakpoint to remove
 * @param errmsg the errmsg populated by the memory access
 *
 * @return 0 on success; non-zero otherwise
 */
int remove_breakpoint(breakpoint *bp, udi_errmsg *errmsg) {
    if ( !bp->in_memory ) return 0;

    int result = write_saved_bytes(bp, errmsg);

    if ( result == 0 ) {
        bp->in_memory = 0;
    }

    return result;
}

/**
 * Removes the breakpoint from memory in preparation for a continue
 *
 * @param bp the breakpoint to remove
 * @param errmsg the errmsg populated by the memory access
 *
 * @return 0 on success; non-zero otherwise
 */
int remove_breakpoint_for_continue(breakpoint *bp, udi_errmsg *errmsg) {

    if ( !bp->in_memory ) return 0;

    // Don't set the in memory flag
    return write_saved_bytes(bp, errmsg);
}

/**
 * Deletes the breakpoint
 *
 * @param bp the breakpoint to delete
 * @param errmsg the errmsg populated by the memory access
 *
 * @return 0 on success; non-zero otherwise
 */
int delete_breakpoint(breakpoint *bp, udi_errmsg *errmsg) {
    int remove_result = remove_breakpoint(bp, errmsg);

    if ( remove_result ) return remove_result;

    unsigned int hash = breakpoint_hash(bp->address);

    breakpoint *tmp_breakpoint = breakpoints[hash];
    breakpoint *prev_breakpoint = breakpoints[hash];

    while ( tmp_breakpoint != NULL ) {
        if ( tmp_breakpoint->address == bp->address ) break;
        prev_breakpoint = tmp_breakpoint;
        tmp_breakpoint = prev_breakpoint->next_breakpoint;
    }

    if ( tmp_breakpoint == NULL || prev_breakpoint == NULL ) {
        udi_set_errmsg(errmsg, "failed to delete breakpoint at %a", bp->address);
        udi_log("%s", errmsg->msg);
        return -1;
    }

    // single element in hash table bucket
    if ( prev_breakpoint == tmp_breakpoint ) {
        breakpoints[hash] = NULL;
    }else{
        prev_breakpoint->next_breakpoint = tmp_breakpoint->next_breakpoint;
    }

    udi_free(tmp_breakpoint);

    return 0;
}

/**
 * Finds an existing breakpoint, given its address
 *
 * @param breakpoint_addr
 *
 * @return the found breakpoint (NULL if not found)
 */
breakpoint *find_breakpoint(uint64_t breakpoint_addr) {
    breakpoint *tmp_breakpoint = breakpoints[breakpoint_hash(breakpoint_addr)];

    while ( tmp_breakpoint != NULL ) {
        if ( tmp_breakpoint->address == breakpoint_addr ) break;
        tmp_breakpoint = tmp_breakpoint->next_breakpoint;
    }

    return tmp_breakpoint;
}

/**
 * @return the protocol version for this execution
 */
udi_version_e get_protocol_version() {
    return UDI_PROTOCOL_VERSION_1;
}

#define CASE_TO_STR(x) case x: return #x

const char *request_type_str(udi_request_type_e req_type) {
    switch(req_type) {
        CASE_TO_STR(UDI_REQ_INVALID);
        CASE_TO_STR(UDI_REQ_CONTINUE);
        CASE_TO_STR(UDI_REQ_READ_MEM);
        CASE_TO_STR(UDI_REQ_WRITE_MEM);
        CASE_TO_STR(UDI_REQ_STATE);
        CASE_TO_STR(UDI_REQ_INIT);
        CASE_TO_STR(UDI_REQ_CREATE_BREAKPOINT);
        CASE_TO_STR(UDI_REQ_INSTALL_BREAKPOINT);
        CASE_TO_STR(UDI_REQ_REMOVE_BREAKPOINT);
        CASE_TO_STR(UDI_REQ_DELETE_BREAKPOINT);
        CASE_TO_STR(UDI_REQ_THREAD_SUSPEND);
        CASE_TO_STR(UDI_REQ_THREAD_RESUME);
        CASE_TO_STR(UDI_REQ_READ_REGISTER);
        CASE_TO_STR(UDI_REQ_WRITE_REGISTER);
        CASE_TO_STR(UDI_REQ_NEXT_INSTRUCTION);
        CASE_TO_STR(UDI_REQ_SINGLE_STEP);
        default: return "UNKNOWN";
    }
}

const char *event_type_str(udi_event_type_e event_type) {
    switch(event_type) {
        CASE_TO_STR(UDI_EVENT_UNKNOWN);
        CASE_TO_STR(UDI_EVENT_ERROR);
        CASE_TO_STR(UDI_EVENT_SIGNAL);
        CASE_TO_STR(UDI_EVENT_BREAKPOINT);
        CASE_TO_STR(UDI_EVENT_THREAD_CREATE);
        CASE_TO_STR(UDI_EVENT_THREAD_DEATH);
        CASE_TO_STR(UDI_EVENT_PROCESS_EXIT);
        CASE_TO_STR(UDI_EVENT_PROCESS_FORK);
        CASE_TO_STR(UDI_EVENT_PROCESS_EXEC);
        CASE_TO_STR(UDI_EVENT_SINGLE_STEP);
        default: return "UNSPECIFIED";
    }
}

const char *arch_str(udi_arch_e arch) {
    switch (arch) {
        CASE_TO_STR(UDI_ARCH_X86);
        CASE_TO_STR(UDI_ARCH_X86_64);
        default: return "UNSPECIFIED";
    }
}

const char *register_str(udi_register_e reg) {
    switch (reg) {
        CASE_TO_STR(UDI_X86_MIN);
        CASE_TO_STR(UDI_X86_GS);
        CASE_TO_STR(UDI_X86_FS);
        CASE_TO_STR(UDI_X86_ES);
        CASE_TO_STR(UDI_X86_DS);
        CASE_TO_STR(UDI_X86_EDI);
        CASE_TO_STR(UDI_X86_ESI);
        CASE_TO_STR(UDI_X86_EBP);
        CASE_TO_STR(UDI_X86_ESP);
        CASE_TO_STR(UDI_X86_EBX);
        CASE_TO_STR(UDI_X86_EDX);
        CASE_TO_STR(UDI_X86_ECX);
        CASE_TO_STR(UDI_X86_EAX);
        CASE_TO_STR(UDI_X86_CS);
        CASE_TO_STR(UDI_X86_SS);
        CASE_TO_STR(UDI_X86_EIP);
        CASE_TO_STR(UDI_X86_FLAGS);
        CASE_TO_STR(UDI_X86_ST0);
        CASE_TO_STR(UDI_X86_ST1);
        CASE_TO_STR(UDI_X86_ST2);
        CASE_TO_STR(UDI_X86_ST3);
        CASE_TO_STR(UDI_X86_ST4);
        CASE_TO_STR(UDI_X86_ST5);
        CASE_TO_STR(UDI_X86_ST6);
        CASE_TO_STR(UDI_X86_ST7);
        CASE_TO_STR(UDI_X86_MAX);
        CASE_TO_STR(UDI_X86_64_MIN);
        CASE_TO_STR(UDI_X86_64_R8);
        CASE_TO_STR(UDI_X86_64_R9);
        CASE_TO_STR(UDI_X86_64_R10);
        CASE_TO_STR(UDI_X86_64_R11);
        CASE_TO_STR(UDI_X86_64_R12);
        CASE_TO_STR(UDI_X86_64_R13);
        CASE_TO_STR(UDI_X86_64_R14);
        CASE_TO_STR(UDI_X86_64_R15);
        CASE_TO_STR(UDI_X86_64_RDI);
        CASE_TO_STR(UDI_X86_64_RSI);
        CASE_TO_STR(UDI_X86_64_RBP);
        CASE_TO_STR(UDI_X86_64_RBX);
        CASE_TO_STR(UDI_X86_64_RDX);
        CASE_TO_STR(UDI_X86_64_RAX);
        CASE_TO_STR(UDI_X86_64_RCX);
        CASE_TO_STR(UDI_X86_64_RSP);
        CASE_TO_STR(UDI_X86_64_RIP);
        CASE_TO_STR(UDI_X86_64_CSGSFS);
        CASE_TO_STR(UDI_X86_64_FLAGS);
        CASE_TO_STR(UDI_X86_64_ST0);
        CASE_TO_STR(UDI_X86_64_ST1);
        CASE_TO_STR(UDI_X86_64_ST2);
        CASE_TO_STR(UDI_X86_64_ST3);
        CASE_TO_STR(UDI_X86_64_ST4);
        CASE_TO_STR(UDI_X86_64_ST5);
        CASE_TO_STR(UDI_X86_64_ST6);
        CASE_TO_STR(UDI_X86_64_ST7);
        CASE_TO_STR(UDI_X86_64_XMM0);
        CASE_TO_STR(UDI_X86_64_XMM1);
        CASE_TO_STR(UDI_X86_64_XMM2);
        CASE_TO_STR(UDI_X86_64_XMM3);
        CASE_TO_STR(UDI_X86_64_XMM4);
        CASE_TO_STR(UDI_X86_64_XMM5);
        CASE_TO_STR(UDI_X86_64_XMM6);
        CASE_TO_STR(UDI_X86_64_XMM7);
        CASE_TO_STR(UDI_X86_64_XMM8);
        CASE_TO_STR(UDI_X86_64_XMM9);
        CASE_TO_STR(UDI_X86_64_XMM10);
        CASE_TO_STR(UDI_X86_64_XMM11);
        CASE_TO_STR(UDI_X86_64_XMM12);
        CASE_TO_STR(UDI_X86_64_XMM13);
        CASE_TO_STR(UDI_X86_64_XMM14);
        CASE_TO_STR(UDI_X86_64_XMM15);
        CASE_TO_STR(UDI_X86_64_MAX);
        default: return "UNSPECIFIED";
    }
}

int validate_register(udi_register_e reg, udi_errmsg *errmsg) {

    int result = 0;
    udi_arch_e arch = get_architecture();
    switch (arch) {
        case UDI_ARCH_X86:
            if (reg <= UDI_X86_MIN || reg >= UDI_X86_MAX) {
                result = -1;
            }
            break;
        case UDI_ARCH_X86_64:
            if (reg <= UDI_X86_64_MIN || reg >= UDI_X86_64_MAX) {
                result = -1;
            }
            break;
    }

    if (result != 0) {
        udi_set_errmsg(errmsg,
                       "invalid register %s for architecture %s",
                       register_str(reg),
                       arch_str(arch));
    }

    return result;
}

static const char * const LEFT_SQ = "[";
static const char * const RIGHT_SQ = "]";
static const char * const COLON = ":";
static const char * const LF = "\n";
static const char * const HEX_PREFIX = "0x";
static const char * const NEG = "-";
static const char * const SPACE = " ";

typedef void (*format_cb)(void *ctx, const void *data, size_t len);

static
void write_log(void *ctx, const void *data, size_t len) {
    USE(ctx);

    write_to(udi_log_fd(), (const uint8_t *)data, len);
}

static
void udi_log_integer(format_cb cb, void *ctx, int value) {
    char buf[10];
    int sign;

    if (value < 0) {
        sign = 1;
        value = -value;
    } else {
        sign = 0;
    }

    int i = 0;
    do {
        buf[i] = (value % 10) + '0';

        i++;
        value = value / 10;
    }while (i < 10 && value > 0);

    if (sign) {
        cb(ctx, NEG, 1);
    }

    for (int j = i-1; j >= 0; j--) {
        cb(ctx, buf + j, 1);
    }
}

static
void udi_log_nibble(format_cb cb, void *ctx, uint8_t nibble) {
    char digit;
    if (nibble < 10) {
        digit = nibble + '0';
    } else {
        digit = (nibble - 10) + 'a';
    }
    cb(ctx, &digit, 1);
}

static
void udi_log_byte(format_cb cb, void *ctx, uint8_t byte) {
    udi_log_nibble(cb, ctx, (byte & 0xf0) >> 4);
    udi_log_nibble(cb, ctx, byte & 0x0f);
}

static
void udi_log_64bit(format_cb cb, void *ctx, uint64_t address) {
    uint8_t shift = 56;
    while (shift > 0) {
        udi_log_byte(cb, ctx, (uint8_t)(address >> shift) & 0xff);
        shift -= 8;
    }
    udi_log_byte(cb, ctx, (uint8_t)(address) & 0xff);
}

static
void udi_log_address(format_cb cb, void *ctx, uint64_t address) {
    cb(ctx, HEX_PREFIX, 2);
    udi_log_64bit(cb, ctx, address);
}

void udi_log_string(format_cb cb, void *ctx, const char *string) {
    size_t len = strlen(string);
    cb(ctx, string, len);
}

static
void udi_log_size_t(format_cb cb, void *ctx, size_t value) {
    udi_log_integer(cb, ctx, (int)value);
}

static
void udi_log_varargs(format_cb cb, void *ctx, const char *format, va_list args_ptr) {

    for (const char *format_ptr = format; *format_ptr; format_ptr++) {
        if (*format_ptr != '%') {
            cb(ctx, format_ptr, 1);
            continue;
        }

        // skip % character
        format_ptr++;

        switch (*format_ptr) {
            case 'a':
                udi_log_address(cb, ctx, va_arg(args_ptr, uint64_t));
                break;
            case 'e':
                udi_log_error(cb, ctx, va_arg(args_ptr, int));
                break;
            case 's':
                udi_log_string(cb, ctx, va_arg(args_ptr, char *));
                break;
            case 'd':
                udi_log_integer(cb, ctx, va_arg(args_ptr, int));
                break;
            case 'b':
                udi_log_byte(cb, ctx, (uint8_t)va_arg(args_ptr, int));
                break;
            case 'l':
                udi_log_size_t(cb, ctx, va_arg(args_ptr, size_t));
                break;
            case 'x':
                udi_log_64bit(cb, ctx, va_arg(args_ptr, uint64_t));
                break;
            default:
                cb(ctx, format_ptr, 1);
                break;
        }
    }
}

void udi_log_formatted(const char *format, const char *file, int line, ...) {
    va_list args_ptr;

    udi_log_lock();

    udi_log_string(write_log, NULL, file);
    write_log(NULL, LEFT_SQ, 1);
    udi_log_integer(write_log, NULL, line);
    write_log(NULL, RIGHT_SQ, 1);
    write_log(NULL, COLON, 1);
    write_log(NULL, SPACE, 1);

    va_start(args_ptr, line);
    udi_log_varargs(write_log, NULL, format, args_ptr);
    va_end(args_ptr);

    write_log(NULL, LF, 1);

    udi_log_unlock();
}

void udi_log_formatted_noprefix(const char *format, ...) {
    va_list args_ptr;

    udi_log_lock();

    va_start(args_ptr, format);
    udi_log_varargs(write_log, NULL, format, args_ptr);
    va_end(args_ptr);

    udi_log_unlock();
}

struct str_ctx {
    char *str;
    size_t size;
    size_t idx;
};

static
void write_string(void *in_ctx, const void *data, size_t len) {
    struct str_ctx *ctx = (struct str_ctx *)in_ctx;

    size_t copy_len;
    if (ctx->idx + len > ctx->size) {
        copy_len = ctx->size - ctx->idx;
    } else {
        copy_len = len;
    }

    if (copy_len > 0) {
        memcpy(ctx->str + ctx->idx, data, copy_len);

        ctx->idx += copy_len;
    }
}

void udi_formatted_str(char *str, size_t size, const char *format, ...) {
    va_list args_ptr;

    struct str_ctx ctx;
    ctx.str = str;
    ctx.size = size-1;
    ctx.idx = 0;

    va_start(args_ptr, format);
    udi_log_varargs(write_string, &ctx, format, args_ptr);
    va_end(args_ptr);

    str[ctx.idx] = '\0';
}

