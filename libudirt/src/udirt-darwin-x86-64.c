/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// UDI RT implementation specific to 64-bit x86 and Darwin

// This needs to be included first to set feature macros
#include "udirt-platform.h"

#include <ucontext.h>
#include <inttypes.h>
#include <errno.h>

#include "udi.h"
#include "udirt.h"
#include "udirt-posix.h"
#include "udirt-x86.h"

typedef struct __darwin_mcontext64 reg_data;

void rewind_pc(void *in_context) {
    ucontext_t *context = (ucontext_t *)in_context;

    context->uc_mcontext->__ss.__rip--;
}

int allocate_context_data(void **context_data) {
    reg_data *data = (reg_data *) udi_malloc(sizeof(reg_data));
    if (data == NULL) {
        *context_data = NULL;
        return ENOMEM;
    }

    *context_data = data;
    return 0;
}

void copy_context(const ucontext_t *src, signal_state *dst) {
    dst->context = *src;

    reg_data *data = (reg_data *)dst->context_data;
    *data = *(dst->context.uc_mcontext);
    dst->context.uc_mcontext = data;
}

void set_pc(ucontext_t *context, unsigned long pc) {
    context->uc_mcontext->__ss.__rip = pc;
}


uint64_t get_pc(const void *input) {
    ucontext_t *context = (ucontext_t *)input;

    return context->uc_mcontext->__ss.__rip;
}

uint64_t get_flags(const void *context) {
    const ucontext_t *u_context = (const ucontext_t *)context;

    return u_context->uc_mcontext->__ss.__rflags;
}

uint64_t get_trap_address(const ucontext_t *context) {

    return context->uc_mcontext->__ss.__rip - 1;
}

int get_exit_argument(const ucontext_t *context,
                      int *status,
                      udi_errmsg *errmsg)
{

    // The exit argument is passed in rax
    *status = (int)context->uc_mcontext->__ss.__rax;
    return RESULT_SUCCESS;
}

int is_accessible_register(ud_type_t reg) {
    switch(reg) {
        case UD_R_AL:
        case UD_R_AH:
        case UD_R_AX:
        case UD_R_EAX:
        case UD_R_RAX:
        case UD_R_CL:
        case UD_R_CH:
        case UD_R_CX:
        case UD_R_ECX:
        case UD_R_RCX:
        case UD_R_DL:
        case UD_R_DH:
        case UD_R_EDX:
        case UD_R_RDX:
        case UD_R_BL:
        case UD_R_BH:
        case UD_R_EBX:
        case UD_R_RBX:
        case UD_R_RIP:
        case UD_R_SPL:
        case UD_R_SP:
        case UD_R_ESP:
        case UD_R_RSP:
        case UD_R_BPL:
        case UD_R_BP:
        case UD_R_EBP:
        case UD_R_RBP:
        case UD_R_SIL:
        case UD_R_SI:
        case UD_R_ESI:
        case UD_R_RSI:
        case UD_R_DIL:
        case UD_R_DI:
        case UD_R_EDI:
        case UD_R_RDI:
        case UD_R_R8B:
        case UD_R_R8W:
        case UD_R_R8D:
        case UD_R_R8:
        case UD_R_R9B:
        case UD_R_R9W:
        case UD_R_R9D:
        case UD_R_R9:
        case UD_R_R10B:
        case UD_R_R10W:
        case UD_R_R10D:
        case UD_R_R10:
        case UD_R_R11B:
        case UD_R_R11W:
        case UD_R_R11D:
        case UD_R_R11:
        case UD_R_R12B:
        case UD_R_R12W:
        case UD_R_R12D:
        case UD_R_R12:
        case UD_R_R13B:
        case UD_R_R13W:
        case UD_R_R13D:
        case UD_R_R13:
        case UD_R_R14B:
        case UD_R_R14W:
        case UD_R_R14D:
        case UD_R_R14:
        case UD_R_R15B:
        case UD_R_R15W:
        case UD_R_R15D:
        case UD_R_R15:
            return 0;
        default:
            return -1;
    }
}

uint64_t get_register_ud_type(ud_type_t reg, const void *v_context) {
    const ucontext_t *context = (const ucontext_t *)v_context;

    switch(reg) {
        case UD_R_AL:
        case UD_R_AH:
        case UD_R_AX:
        case UD_R_EAX:
        case UD_R_RAX:
            return context->uc_mcontext->__ss.__rax;

        case UD_R_CL:
        case UD_R_CH:
        case UD_R_CX:
        case UD_R_ECX:
        case UD_R_RCX:
            return context->uc_mcontext->__ss.__rcx;

        case UD_R_DL:
        case UD_R_DH:
        case UD_R_EDX:
        case UD_R_RDX:
            return context->uc_mcontext->__ss.__rdx;

        case UD_R_BL:
        case UD_R_BH:
        case UD_R_EBX:
        case UD_R_RBX:
            return context->uc_mcontext->__ss.__rbx;

        case UD_R_RIP:
            return context->uc_mcontext->__ss.__rip;

        case UD_R_SPL:
        case UD_R_SP:
        case UD_R_ESP:
        case UD_R_RSP:
            return context->uc_mcontext->__ss.__rsp;

        case UD_R_BPL:
        case UD_R_BP:
        case UD_R_EBP:
        case UD_R_RBP:
            return context->uc_mcontext->__ss.__rbp;

        case UD_R_SIL:
        case UD_R_SI:
        case UD_R_ESI:
        case UD_R_RSI:
            return context->uc_mcontext->__ss.__rsi;

        case UD_R_DIL:
        case UD_R_DI:
        case UD_R_EDI:
        case UD_R_RDI:
            return context->uc_mcontext->__ss.__rdi;

        case UD_R_R8B:
        case UD_R_R8W:
        case UD_R_R8D:
        case UD_R_R8:
            return context->uc_mcontext->__ss.__r8;

        case UD_R_R9B:
        case UD_R_R9W:
        case UD_R_R9D:
        case UD_R_R9:
            return context->uc_mcontext->__ss.__r9;

        case UD_R_R10B:
        case UD_R_R10W:
        case UD_R_R10D:
        case UD_R_R10:
            return context->uc_mcontext->__ss.__r10;

        case UD_R_R11B:
        case UD_R_R11W:
        case UD_R_R11D:
        case UD_R_R11:
            return context->uc_mcontext->__ss.__r11;

        case UD_R_R12B:
        case UD_R_R12W:
        case UD_R_R12D:
        case UD_R_R12:
            return context->uc_mcontext->__ss.__r12;

        case UD_R_R13B:
        case UD_R_R13W:
        case UD_R_R13D:
        case UD_R_R13:
            return context->uc_mcontext->__ss.__r13;

        case UD_R_R14B:
        case UD_R_R14W:
        case UD_R_R14D:
        case UD_R_R14:
            return context->uc_mcontext->__ss.__r14;

        case UD_R_R15B:
        case UD_R_R15W:
        case UD_R_R15D:
        case UD_R_R15:
            return context->uc_mcontext->__ss.__r15;

        default:
            udi_abort();
            return (uint64_t)-1;
    }
}

static
int get_register_address(udi_register_e reg, const ucontext_t *context, uint64_t **address) {
    switch (reg) {
        case UDI_X86_GS:
        case UDI_X86_FS:
        case UDI_X86_ES:
        case UDI_X86_DS:
        case UDI_X86_EDI:
        case UDI_X86_ESI:
        case UDI_X86_EBP:
        case UDI_X86_ESP:
        case UDI_X86_EBX:
        case UDI_X86_EDX:
        case UDI_X86_ECX:
        case UDI_X86_EAX:
        case UDI_X86_CS:
        case UDI_X86_SS:
        case UDI_X86_EIP:
        case UDI_X86_FLAGS:
        case UDI_X86_ST0:
        case UDI_X86_ST1:
        case UDI_X86_ST2:
        case UDI_X86_ST3:
        case UDI_X86_ST4:
        case UDI_X86_ST5:
        case UDI_X86_ST6:
        case UDI_X86_ST7:
            // TODO X86 support
            *address = NULL;
            return -1;

        case UDI_X86_64_R8:
            *address = &(context->uc_mcontext->__ss.__r8);
            break;

        case UDI_X86_64_R9:
            *address = &(context->uc_mcontext->__ss.__r9);
            break;

        case UDI_X86_64_R10:
            *address = &(context->uc_mcontext->__ss.__r10);
            break;

        case UDI_X86_64_R11:
            *address = &(context->uc_mcontext->__ss.__r11);
            break;

        case UDI_X86_64_R12:
            *address = &(context->uc_mcontext->__ss.__r12);
            break;

        case UDI_X86_64_R13:
            *address = &(context->uc_mcontext->__ss.__r13);
            break;

        case UDI_X86_64_R14:
            *address = &(context->uc_mcontext->__ss.__r14);
            break;

        case UDI_X86_64_R15:
            *address = &(context->uc_mcontext->__ss.__r15);
            break;

        case UDI_X86_64_RDI:
            *address = &(context->uc_mcontext->__ss.__rdi);
            break;

        case UDI_X86_64_RSI:
            *address = &(context->uc_mcontext->__ss.__rsi);
            break;

        case UDI_X86_64_RBP:
            *address = &(context->uc_mcontext->__ss.__rbp);
            break;

        case UDI_X86_64_RBX:
            *address = &(context->uc_mcontext->__ss.__rbx);
            break;

        case UDI_X86_64_RDX:
            *address = &(context->uc_mcontext->__ss.__rdx);
            break;

        case UDI_X86_64_RAX:
            *address = &(context->uc_mcontext->__ss.__rax);
            break;

        case UDI_X86_64_RCX:
            *address = &(context->uc_mcontext->__ss.__rcx);
            break;

        case UDI_X86_64_RSP:
            *address = &(context->uc_mcontext->__ss.__rsp);
            break;

        case UDI_X86_64_RIP:
            *address = &(context->uc_mcontext->__ss.__rip);
            break;

        case UDI_X86_64_FLAGS:
            *address = &(context->uc_mcontext->__ss.__rflags);
            break;

        case UDI_X86_64_CSGSFS:
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
        default:
            // TODO support for FP registers
            *address = NULL;
            return -1;
    }

    return 0;
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

    uint64_t *address = NULL;
    if (get_register_address(reg, u_context, &address) != 0) {
        udi_set_errmsg(errmsg,
                       "invalid register %s",
                       register_str(reg));
        return -1;
    }

    *value = *address;
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

    uint64_t *address = NULL;
    if (get_register_address(reg, u_context, &address) != 0) {
        udi_set_errmsg(errmsg,
                       "invalid register %s",
                       register_str(reg));
        return -1;
    }

    *address = value;
    return 0;
}
