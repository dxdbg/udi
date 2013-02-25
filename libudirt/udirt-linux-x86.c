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

// UDI RT implementation specific to x86 and linux

// This needs to be included first to set feature macros
#include "udirt-platform.h"

#include <ucontext.h>
#include <inttypes.h>

#include "udi.h"
#include "udirt.h"
#include "udirt-posix.h"
#include "udirt-x86.h"
#include "udirt-posix-x86.h"

/**
 * Given the context, rewinds the PC to account for a hit breakpoint
 *
 * @param context the context containing the current PC value
 */
void rewind_pc(ucontext_t *context) {
    if (__WORDSIZE == 64) {
        context->uc_mcontext.gregs[X86_64_RIP_OFFSET]--;
    }else{
        context->uc_mcontext.gregs[X86_EIP_OFFSET]--;
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
        context->uc_mcontext.gregs[X86_64_RIP_OFFSET] = pc;
    }else{
        context->uc_mcontext.gregs[X86_EIP_OFFSET] = pc;
    }
}

/**
 * Given the context, gets the pc
 *
 * @param context the context containing the current PC value
 * 
 * @return the PC contained in the context
 */
unsigned long get_pc(const ucontext_t *context) {
    if (__WORDSIZE == 64) {
        return context->uc_mcontext.gregs[X86_64_RIP_OFFSET];
    }
        
    return context->uc_mcontext.gregs[X86_EIP_OFFSET];
}

/**
 * Given the context, gets the flags register
 *
 * @param context the context containing the flags register
 *
 * @return the flags register in the context
 */
unsigned long get_flags(const void *context) {
    const ucontext_t *u_context = (const ucontext_t *)context;

    if (__WORDSIZE == 64) {
        return u_context->uc_mcontext.gregs[X86_64_FLAGS_OFFSET];
    }

    return u_context->uc_mcontext.gregs[X86_FLAGS_OFFSET];
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
        return (udi_address)(unsigned long)context->uc_mcontext.gregs[X86_64_RIP_OFFSET] - 1;
    }

    return (udi_address)(unsigned long)context->uc_mcontext.gregs[X86_EIP_OFFSET] - 1;
}

/**
 * Determines the argument to the exit function, given the context at which the exit breakpoint
 * was hit
 *
 * @param context the current context
 * @param errmsg the error message populated by the memory access
 *
 * @return the exit result
 */
exit_result get_exit_argument(const ucontext_t *context, udi_errmsg *errmsg) {
    exit_result ret;
    ret.failure = 0;

    if (__WORDSIZE == 64) {
        // The exit argument is passed in rax
        ret.status = (int)context->uc_mcontext.gregs[X86_64_RAX_OFFSET];
    }else{
        // The exit argument is the first parameter on the stack

        // Get the stack pointer
        unsigned long sp;

        sp = (unsigned long)context->uc_mcontext.gregs[X86_ESP_OFFSET];

        int word_length = sizeof(unsigned long);

        // skip return address
        sp += word_length;

        int read_result = read_memory(&(ret.status), (const void *)sp, sizeof(int), errmsg);
        if ( read_result != 0 ) {
            udi_printf("failed to retrieve exit status off of the stack at 0x%lx\n",
                    sp);
            snprintf(errmsg->msg, errmsg->size,
                    "failed to retrieve exit status off of the stack at 0x%lx: %s",
                    sp, get_mem_errstr());
            ret.failure = read_result;
        }
    }

    return ret;
}

/**
 * @param reg the register
 *
 * @return the offset in the context array
 */
static
int get_reg_context_offset(ud_type_t reg) {
    if ( __WORDSIZE == 64 ) {
        switch(reg) {
            case UD_R_AL:
            case UD_R_AH:
            case UD_R_AX:
            case UD_R_EAX:
            case UD_R_RAX:
                return X86_64_RAX_OFFSET;
            case UD_R_CL:
            case UD_R_CH:
            case UD_R_CX:
            case UD_R_ECX:
            case UD_R_RCX:
                return X86_64_RCX_OFFSET;
            case UD_R_DL:
            case UD_R_DH:
            case UD_R_EDX:
            case UD_R_RDX:
                return X86_64_RDX_OFFSET;
            case UD_R_BL:
            case UD_R_BH:
            case UD_R_EBX:
            case UD_R_RBX:
                return X86_64_RBX_OFFSET;
            case UD_R_RIP:
                return X86_64_RIP_OFFSET;
            case UD_R_SPL:
            case UD_R_SP:
            case UD_R_ESP:
            case UD_R_RSP:
                return X86_64_RSP_OFFSET;
            case UD_R_BPL:
            case UD_R_BP:
            case UD_R_EBP:
            case UD_R_RBP:
                return X86_64_RBP_OFFSET;
            case UD_R_SIL:
            case UD_R_SI:
            case UD_R_ESI:
            case UD_R_RSI:
                return X86_64_RSI_OFFSET;
            case UD_R_DIL:
            case UD_R_DI:
            case UD_R_EDI:
            case UD_R_RDI:
                return X86_64_RDI_OFFSET;
            case UD_R_R8B:
            case UD_R_R8W:
            case UD_R_R8D:
            case UD_R_R8:
                return X86_64_R8_OFFSET;
            case UD_R_R9B:
            case UD_R_R9W:
            case UD_R_R9D:
            case UD_R_R9:
                return X86_64_R9_OFFSET;
            case UD_R_R10B:
            case UD_R_R10W:
            case UD_R_R10D:
            case UD_R_R10:
                return X86_64_R10_OFFSET;
            case UD_R_R11B:
            case UD_R_R11W:
            case UD_R_R11D:
            case UD_R_R11:
                return X86_64_R11_OFFSET;
            case UD_R_R12B:
            case UD_R_R12W:
            case UD_R_R12D:
            case UD_R_R12:
                return X86_64_R12_OFFSET;
            case UD_R_R13B:
            case UD_R_R13W:
            case UD_R_R13D:
            case UD_R_R13:
                return X86_64_R13_OFFSET;
            case UD_R_R14B:
            case UD_R_R14W:
            case UD_R_R14D:
            case UD_R_R14:
                return X86_64_R14_OFFSET;
            case UD_R_R15B:
            case UD_R_R15W:
            case UD_R_R15D:
            case UD_R_R15:
                return X86_64_R15_OFFSET;
            default:
                return -1;
        }
    }else{
        switch(reg) {
            default:
                return -1;
        }
    }
}

/**
 * @param reg the register
 *
 * @return 0 if it isn't accessible; non-zero if it is
 */
int is_accessible_register(ud_type_t reg) {
    return get_reg_context_offset(reg) != -1;
}

/**
 * Gets the specified register from the context
 *
 * @param reg the register to retrieve
 * @param context the context
 *
 * @return the value from the register
 */
unsigned long get_register_ud_type(ud_type_t reg, const void *context) {
    const ucontext_t *u_context = (const ucontext_t *)context;

    int offset = get_reg_context_offset(reg);

    if ( offset == -1 ) {
        udi_abort(__FILE__, __LINE__);
    }

   if (__WORDSIZE == 64) {
        return u_context->uc_mcontext.gregs[offset];
    }
        
    return u_context->uc_mcontext.gregs[offset];
}
