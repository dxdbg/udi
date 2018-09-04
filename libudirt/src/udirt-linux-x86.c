/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

#ifndef REG_GS
#define REG_GS -1
#endif
static int X86_GS_OFFSET = REG_GS;

#ifndef REG_FS
#define REG_FS -1
#endif
static int X86_FS_OFFSET = REG_FS;

#ifndef REG_ES
#define REG_ES -1
#endif
static int X86_ES_OFFSET = REG_ES;

#ifndef REG_DS
#define REG_DS -1
#endif
static int X86_DS_OFFSET = REG_DS;

#ifndef REG_EDI
#define REG_EDI -1
#endif
static int X86_EDI_OFFSET = REG_EDI;

#ifndef REG_ESI
#define REG_ESI -1
#endif
static int X86_ESI_OFFSET = REG_ESI;

#ifndef REG_EBP
#define REG_EBP -1
#endif
static int X86_EBP_OFFSET = REG_EBP;

#ifndef REG_ESP
#define REG_ESP -1
#endif
static int X86_ESP_OFFSET = REG_ESP;

#ifndef REG_EBX
#define REG_EBX -1
#endif
static int X86_EBX_OFFSET = REG_EBX;

#ifndef REG_EDX
#define REG_EDX -1
#endif
static int X86_EDX_OFFSET = REG_EDX;

#ifndef REG_ECX
#define REG_ECX -1
#endif
static int X86_ECX_OFFSET = REG_ECX;

#ifndef REG_EAX
#define REG_EAX -1
#endif
static int X86_EAX_OFFSET = REG_EAX;

#ifndef REG_CS
#define REG_CS -1
#endif
static int X86_CS_OFFSET = REG_CS;

#ifndef REG_SS
#define REG_SS -1
#endif
static int X86_SS_OFFSET = REG_SS;

#ifndef REG_EIP
#define REG_EIP -1
#endif
static int X86_EIP_OFFSET = REG_EIP;

#ifndef REG_EFL
#define REG_EFL -1
#endif
static int X86_FLAGS_OFFSET = REG_EFL;

// x86_64

#ifndef REG_R8
#define REG_R8 -1
#endif
static int X86_64_R8_OFFSET = REG_R8;

#ifndef REG_R9
#define REG_R9 -1
#endif
static int X86_64_R9_OFFSET = REG_R9;

#ifndef REG_R10
#define REG_R10 -1
#endif
static int X86_64_R10_OFFSET = REG_R10;

#ifndef REG_R11
#define REG_R11 -1
#endif
static int X86_64_R11_OFFSET = REG_R11;

#ifndef REG_R12
#define REG_R12 -1
#endif
static int X86_64_R12_OFFSET = REG_R12;

#ifndef REG_R13
#define REG_R13 -1
#endif
static int X86_64_R13_OFFSET = REG_R13;

#ifndef REG_R14
#define REG_R14 -1
#endif
static int X86_64_R14_OFFSET = REG_R14;

#ifndef REG_R15
#define REG_R15 -1
#endif
static int X86_64_R15_OFFSET = REG_R15;

#ifndef REG_RDI
#define REG_RDI -1
#endif
static int X86_64_RDI_OFFSET = REG_RDI;

#ifndef REG_RSI
#define REG_RSI -1
#endif
static int X86_64_RSI_OFFSET = REG_RSI;

#ifndef REG_RBP
#define REG_RBP -1
#endif
static int X86_64_RBP_OFFSET = REG_RBP;

#ifndef REG_RBX
#define REG_RBX -1
#endif
static int X86_64_RBX_OFFSET = REG_RBX;

#ifndef REG_RDX
#define REG_RDX -1
#endif
static int X86_64_RDX_OFFSET = REG_RDX;

#ifndef REG_RAX
#define REG_RAX -1
#endif
static int X86_64_RAX_OFFSET = REG_RAX;

#ifndef REG_RCX
#define REG_RCX -1
#endif
static int X86_64_RCX_OFFSET = REG_RCX;

#ifndef REG_RSP
#define REG_RSP -1
#endif
static int X86_64_RSP_OFFSET = REG_RSP;

#ifndef REG_RIP
#define REG_RIP -1
#endif
static int X86_64_RIP_OFFSET = REG_RIP;

#ifndef REG_CSGSFS
#define REG_CSGSFS -1
#endif
static int X86_64_CSGSFS_OFFSET = REG_CSGSFS;

static int X86_64_FLAGS_OFFSET = REG_EFL;

void rewind_pc(void *in_context) {
    ucontext_t *context = (ucontext_t *)in_context;

    if (__WORDSIZE == 64) {
        context->uc_mcontext.gregs[X86_64_RIP_OFFSET]--;
    }else{
        context->uc_mcontext.gregs[X86_EIP_OFFSET]--;
    }
}

int allocate_context_data(void **context_data) {
    *context_data = NULL;
    return 0;
}

void copy_context(const ucontext_t *src, signal_state *dst) {
    dst->context = *src;
}

void set_pc(ucontext_t *context, unsigned long pc) {
    if (__WORDSIZE == 64) {
        context->uc_mcontext.gregs[X86_64_RIP_OFFSET] = pc;
    }else{
        context->uc_mcontext.gregs[X86_EIP_OFFSET] = pc;
    }
}


uint64_t get_pc(const void *input) {
    ucontext_t *context = (ucontext_t *)input;
    if (__WORDSIZE == 64) {
        return context->uc_mcontext.gregs[X86_64_RIP_OFFSET];
    }

    return context->uc_mcontext.gregs[X86_EIP_OFFSET];
}

unsigned long get_flags(const void *context) {
    const ucontext_t *u_context = (const ucontext_t *)context;

    if (__WORDSIZE == 64) {
        return u_context->uc_mcontext.gregs[X86_64_FLAGS_OFFSET];
    }

    return u_context->uc_mcontext.gregs[X86_FLAGS_OFFSET];
}

uint64_t get_trap_address(const ucontext_t *context) {
    if (__WORDSIZE == 64) {
        return (uint64_t)context->uc_mcontext.gregs[X86_64_RIP_OFFSET] - 1;
    }

    return (uint64_t)context->uc_mcontext.gregs[X86_EIP_OFFSET] - 1;
}

int get_exit_argument(const ucontext_t *context,
                      int *status,
                      udi_errmsg *errmsg)
{
    int result;
    if (__WORDSIZE == 64) {
        // The exit argument is passed in rax
        *status = (int)context->uc_mcontext.gregs[X86_64_RAX_OFFSET];
        result = RESULT_SUCCESS;
    }else{
        // The exit argument is the first parameter on the stack

        // Get the stack pointer
        uint64_t sp;

        sp = (uint64_t)context->uc_mcontext.gregs[X86_ESP_OFFSET];

        int word_length = sizeof(unsigned long);

        // skip return address
        sp += word_length;

        int read_result = read_memory(status, (const uint8_t *)sp, sizeof(int), errmsg);
        if ( read_result != 0 ) {
            udi_log("failed to retrieve exit status off of the stack at %a",
                    sp);
            udi_set_errmsg(errmsg,
                           "failed to retrieve exit status off of the stack at %a: %s",
                           sp,
                           get_mem_errstr());
            result = RESULT_ERROR;
        }
        result = RESULT_SUCCESS;
    }

    return result;
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

int is_accessible_register(ud_type_t reg) {
    return get_reg_context_offset(reg) != -1;
}

unsigned long get_register_ud_type(ud_type_t reg, const void *context) {
    const ucontext_t *u_context = (const ucontext_t *)context;

    int offset = get_reg_context_offset(reg);

    if ( offset == -1 ) {
        udi_abort();
    }

    return u_context->uc_mcontext.gregs[offset];
}

#define REG_CASE(name) case UDI_##name: return name##_OFFSET

/**
 * Gets the index into the context for the register
 *
 * @param reg the register
 *
 * @return the register index
 * @return -2 if the register is unknown
 * @return -1 if the register corresponds to a FP register
 */
static
int get_udi_reg_context_offset(udi_register_e reg) {
    switch (reg) {
        REG_CASE(X86_GS);
        REG_CASE(X86_FS);
        REG_CASE(X86_ES);
        REG_CASE(X86_DS);
        REG_CASE(X86_EDI);
        REG_CASE(X86_ESI);
        REG_CASE(X86_EBP);
        REG_CASE(X86_ESP);
        REG_CASE(X86_EBX);
        REG_CASE(X86_EDX);
        REG_CASE(X86_ECX);
        REG_CASE(X86_EAX);
        REG_CASE(X86_CS);
        REG_CASE(X86_SS);
        REG_CASE(X86_EIP);
        REG_CASE(X86_FLAGS);
        REG_CASE(X86_64_R8);
        REG_CASE(X86_64_R9);
        REG_CASE(X86_64_R10);
        REG_CASE(X86_64_R11);
        REG_CASE(X86_64_R12);
        REG_CASE(X86_64_R13);
        REG_CASE(X86_64_R14);
        REG_CASE(X86_64_R15);
        REG_CASE(X86_64_RDI);
        REG_CASE(X86_64_RSI);
        REG_CASE(X86_64_RBP);
        REG_CASE(X86_64_RBX);
        REG_CASE(X86_64_RDX);
        REG_CASE(X86_64_RAX);
        REG_CASE(X86_64_RCX);
        REG_CASE(X86_64_RSP);
        REG_CASE(X86_64_RIP);
        REG_CASE(X86_64_CSGSFS);
        REG_CASE(X86_64_FLAGS);
        case UDI_X86_ST0:
        case UDI_X86_ST1:
        case UDI_X86_ST2:
        case UDI_X86_ST3:
        case UDI_X86_ST4:
        case UDI_X86_ST5:
        case UDI_X86_ST6:
        case UDI_X86_ST7:
        case UDI_X86_64_ST0:
        case UDI_X86_64_ST1:
        case UDI_X86_64_ST2:
        case UDI_X86_64_ST3:
        case UDI_X86_64_ST4:
        case UDI_X86_64_ST5:
        case UDI_X86_64_ST6:
        case UDI_X86_64_ST7:
        case UDI_X86_64_XMM0:
        case UDI_X86_64_XMM1:
        case UDI_X86_64_XMM2:
        case UDI_X86_64_XMM3:
        case UDI_X86_64_XMM4:
        case UDI_X86_64_XMM5:
        case UDI_X86_64_XMM6:
        case UDI_X86_64_XMM7:
        case UDI_X86_64_XMM8:
        case UDI_X86_64_XMM9:
        case UDI_X86_64_XMM10:
        case UDI_X86_64_XMM11:
        case UDI_X86_64_XMM12:
        case UDI_X86_64_XMM13:
        case UDI_X86_64_XMM14:
        case UDI_X86_64_XMM15:
            return -1;
        default:
            return -2;
    }
}

static
int get_fp_register(udi_register_e reg,
                    uint64_t *value,
                    const ucontext_t *context)
{

    /** TODO need to rethink how floating point values are retrieved
    switch (reg) {
#ifndef __x86_64__
        case UDI_X86_ST0: *value = context->uc_mcontext.fpregs->_st[0];
        case UDI_X86_ST1: *value = context->uc_mcontext.fpregs->_st[1];
        case UDI_X86_ST2: *value = context->uc_mcontext.fpregs->_st[2];
        case UDI_X86_ST3: *value = context->uc_mcontext.fpregs->_st[3];
        case UDI_X86_ST4: *value = context->uc_mcontext.fpregs->_st[4];
        case UDI_X86_ST5: *value = context->uc_mcontext.fpregs->_st[5];
        case UDI_X86_ST6: *value = context->uc_mcontext.fpregs->_st[6];
        case UDI_X86_ST7: *value = context->uc_mcontext.fpregs->_st[7];
#else // __x86_64__
        case UDI_X86_64_ST0: *value = context->uc_mcontext.fpregs->_st[0];
        case UDI_X86_64_ST1: *value = context->uc_mcontext.fpregs->_st[1];
        case UDI_X86_64_ST2: *value = context->uc_mcontext.fpregs->_st[2];
        case UDI_X86_64_ST3: *value = context->uc_mcontext.fpregs->_st[3];
        case UDI_X86_64_ST4: *value = context->uc_mcontext.fpregs->_st[4];
        case UDI_X86_64_ST5: *value = context->uc_mcontext.fpregs->_st[5];
        case UDI_X86_64_ST6: *value = context->uc_mcontext.fpregs->_st[6];
        case UDI_X86_64_ST7: *value = context->uc_mcontext.fpregs->_st[7];
        case UDI_X86_64_XMM0: *value = context->
        case UDI_X86_64_XMM1:
        case UDI_X86_64_XMM2:
        case UDI_X86_64_XMM3:
        case UDI_X86_64_XMM4:
        case UDI_X86_64_XMM5:
        case UDI_X86_64_XMM6:
        case UDI_X86_64_XMM7:
        case UDI_X86_64_XMM8:
        case UDI_X86_64_XMM9:
        case UDI_X86_64_XMM10:
        case UDI_X86_64_XMM11:
        case UDI_X86_64_XMM12:
        case UDI_X86_64_XMM13:
        case UDI_X86_64_XMM14:
        case UDI_X86_64_XMM15:
#endif
        default:
            return -1;
    }
    */

    return -1;
}

int get_register(udi_register_e reg,
                 udi_errmsg *errmsg,
                 uint64_t *value,
                 const void *context)
{
    const ucontext_t *u_context = (const ucontext_t *)context;

    if (validate_register(reg, errmsg)) {
        return -1;
    }

    int offset = get_udi_reg_context_offset(reg);
    if (offset >= 0 ) {
        *value = (uint64_t)u_context->uc_mcontext.gregs[offset];
    }else if (offset == -1) {
        if ( get_fp_register(reg, value, u_context) != 0 ) {
            offset = -2;
        }
    }

    if (offset < -1) {
        udi_set_errmsg(errmsg, "invalid register %d", reg);
        return -1;
    }

    return 0;
}

int set_register(udi_register_e reg,
                 udi_errmsg *errmsg,
                 uint64_t value,
                 void *context)
{
    ucontext_t *u_context = (ucontext_t *)context;

    if (validate_register(reg, errmsg)) {
        return -1;
    }

    int offset = get_udi_reg_context_offset(reg);
    if (offset >= 0 ) {
        u_context->uc_mcontext.gregs[offset] = (unsigned long)value;
    }else if (offset == -1) {
        // TODO set fp register
    }

    if (offset < -1) {
        udi_set_errmsg(errmsg, "invalid register %d", reg);
        return -1;
    }

    return 0;
}
