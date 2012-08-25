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
#include <inttypes.h>

#include "udi.h"
#include "udirt.h"
#include "udirt-posix.h"
#include "udirt-x86.h"

// Exit prologue parsing

// Common on x86 with gcc
static const unsigned char PUSH_EBP = 0x55;

// Common on x86_64 with gcc
static const unsigned char LEA_RIP_BYTE1 = 0x48;
static const unsigned char LEA_RIP_BYTE2 = 0x8d;
static const unsigned char LEA_RIP_BYTE3 = 0x35;

#define X86_EXIT_INST_LENGTH 1
#define X86_64_EXIT_INST_LENGTH 7
#define MAX_EXIT_INST_LENGTH X86_64_EXIT_INST_LENGTH

/**
 * Given the context, rewinds the PC to account for a hit breakpoint
 *
 * @param context the context containing the current PC value
 */
void rewind_pc(ucontext_t *context) {
    if (__WORDSIZE == 64) {
        context->uc_mcontext.gregs[RIP_OFFSET]--;
    }else{
        context->uc_mcontext.gregs[EIP_OFFSET]--;
    }
}

/**
 * Given the context, sets the pc to the supplied value
 *
 * @param context the context containing the current PC value
 * @param pc the new pc value
 */
void set_pc(ucontext_t *context, unsigned long pc) {
    if (__WORDSIZE == 64) {
        context->uc_mcontext.gregs[RIP_OFFSET] = pc;
    }else{
        context->uc_mcontext.gregs[EIP_OFFSET] = pc;
    }
}

/**
 * Given the context, gets the pc
 *
 * @param context the context containing the current PC value
 * 
 * @return the PC contained in the context
 */
unsigned long get_pc(ucontext_t *context) {
    if (__WORDSIZE == 64) {
        return context->uc_mcontext.gregs[RIP_OFFSET];
    }
        
    return context->uc_mcontext.gregs[EIP_OFFSET];
}

/**
 * Given the context, calculates the address at which a trap occurred at.
 *
 * @param context the context containing the current PC value
 *
 * @return the computed address
 */
udi_address get_trap_address(const ucontext_t *context) {
    if (__WORDSIZE == 64) {
        return (udi_address)(unsigned long)context->uc_mcontext.gregs[RIP_OFFSET] - 1;
    }

    return (udi_address)(unsigned long)context->uc_mcontext.gregs[EIP_OFFSET] - 1;
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

    if (__WORDSIZE == 64) {
        // The exit argument is passed in rax
        ret.status = (int)context->uc_mcontext.gregs[RAX_OFFSET];
    }else{
        // The exit argument is the first parameter on the stack

        // Get the stack pointer
        unsigned long sp;

        sp = (unsigned long)context->uc_mcontext.gregs[ESP_OFFSET];

        int word_length = sizeof(unsigned long);

        // skip return address
        sp += word_length;

        int read_result = read_memory(&(ret.status), (const void *)sp, sizeof(int),
                errmsg, errmsg_size);
        if ( read_result != 0 ) {
            udi_printf("failed to retrieve exit status off of the stack at 0x%lx\n",
                    sp);
            snprintf(errmsg, errmsg_size, "failed to retrieve exit status off of the stack at 0x%"PRIx64": %s",
                    sp, get_mem_errstr());
            ret.failure = read_result;
        }
    }

    return ret;
}

/**
 * Gets the control flow successor for the current instruction
 *
 * @param context the context containing the registers
 * @param errmsg the error message populated on error
 * @param errmsg_size the max size of the error message
 *
 * @return the address of the instruction, or 0 on error
 */
unsigned long get_ctf_successor_context(const ucontext_t *context, char *errmsg, unsigned int errmsg_size) {
}
