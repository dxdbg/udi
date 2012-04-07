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

// UDI RT implementation specific to x86 and linux

// This needs to be included first to set feature macros
#include "udirt-platform.h"

#include <ucontext.h>

#include "udi.h"
#include "udirt.h"
#include "udirt-posix.h"
#include "udirt-x86.h"

static const unsigned char PUSH_EBP = 0x55;

/**
 * Given the context, rewinds the PC to account for a hit breakpoint
 *
 * @param context the context containing the current PC value
 */
void rewind_pc(ucontext_t *context) {
    context->uc_mcontext.gregs[REG_EIP]--;
}

/**
 * Given the context, calculates the address at which a trap occurred at.
 *
 * @param context the context containing the current PC value
 *
 * @return the computed address
 */
udi_address get_trap_address(const ucontext_t *context) {
    return (udi_address)(unsigned long)context->uc_mcontext.gregs[REG_EIP] - 1;
}

/**
 * Determines the instruction length of the first instruction of the specified
 * exit function
 *
 * @param exit_func a pointer to the exit function
 * @param errmsg the errmsg populated by the memory access
 * @param errmsg_size the maximum size of the error message
 *
 * @return positive value on success; non-positive otherwise
 */
int get_exit_inst_length(void (*exit_func)(int), char *errmsg, unsigned int errmsg_size) {
    // Use heuristics to determine the instruction length

    unsigned char possible_inst[1]; // this should be the maximum instruction length

    int read_result = read_memory(possible_inst, (const void *)exit_func, sizeof(possible_inst),
            errmsg, errmsg_size);
    if ( read_result != 0 ) {
        udi_printf("failed to read instructions from exit function at 0x%lx\n",
                (unsigned long)exit_func);
        return -1;
    }

    if ( possible_inst[0] == PUSH_EBP ) {
        return 1;
    }

    return -1;
}

/**
 * Determines the argument to the exit function, given the context at which the exit breakpoint
 * was hit
 *
 * @param context the current context
 * @param errmsg the error message populated by the memory access
 * @param errmsg_size the maximum size of the error message
 *
 * @return the exit result
 */
exit_result get_exit_argument(const ucontext_t *context, char *errmsg, unsigned int errmsg_size) {
    exit_result ret;
    ret.failure = 0;

    // The exit argument is the first parameter on the stack

    // Get the stack pointer
    unsigned long sp = (unsigned long)context->uc_mcontext.gregs[REG_ESP];

    int word_length = sizeof(unsigned long);

    // skip return address
    sp += word_length;

    int read_result = read_memory(&(ret.status), (const void *)sp, sizeof(int),
            errmsg, errmsg_size);
    if ( read_result != 0 ) {
        udi_printf("failed to retrieve exit status off of the stack at 0x%lx\n",
                sp);
        ret.failure = read_result;
    }

    return ret;
}
