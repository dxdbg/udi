/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// UDI RT implementation specific to x86 and x86_64

#include "udirt.h"
#include "udirt-x86.h"

#include <inttypes.h>

#include <udis86.h>

static const unsigned char BREAKPOINT_INSN = 0xcc;

/**
 * Writes the breakpoint instruction for the specified breakpoint
 *
 * @param bp the breakpoint
 * @param errmsg the errmsg populated by the memory access
 *
 * @return non-zero on failure
 */
int write_breakpoint_instruction(breakpoint *bp, udi_errmsg *errmsg) {
    if ( bp->in_memory ) return 0;

    int result = read_memory(bp->saved_bytes, (const uint8_t *)(size_t)bp->address,
            sizeof(BREAKPOINT_INSN), errmsg);
    if( result != 0 ) {
        udi_log("failed to save original bytes at %a",
                bp->address);
        return result;
    }

    result = write_memory((void *)(uintptr_t)bp->address,
                          &BREAKPOINT_INSN,
                          sizeof(BREAKPOINT_INSN), errmsg);
    if ( result != 0 ) {
        udi_log("failed to install breakpoint at %a",
                bp->address);
    }

    return result;
}

/**
 * Writes the saved bytes for the breakpoint into memory
 *
 * @param bp the breakpoint that stores the saved bytes
 * @param errmsg the error message populated by the memory access
 *
 * @return non-zero on failure
 */
int write_saved_bytes(breakpoint *bp, udi_errmsg *errmsg) {
    if ( !bp->in_memory ) return 0;

    int result = write_memory((void *)(uintptr_t)bp->address,
                              bp->saved_bytes,
                              sizeof(BREAKPOINT_INSN), errmsg);

    if ( result != 0 ) {
        udi_log("failed to remove breakpoint at %a", bp->address);
    }

    return result;
}

/**
 * Gets the architecture of this process
 *
 * @return the architecture enum
 */
udi_arch_e get_architecture() {
    return (__WORDSIZE == 64 ? UDI_ARCH_X86_64 : UDI_ARCH_X86);
}

// x86 instruction analysis functions

/**
 * Computes the location contained in the operand relative to the specified pc
 *
 * @param op the operand
 * @param pc the pc
 *
 * @return the relative immediate
 */
static
unsigned long compute_relative_location(struct ud_operand *op, unsigned long pc) {
    switch (op->size) {
        case 8:
           return (unsigned long)(pc + op->lval.sbyte);
        case 16:
           return (unsigned long)(pc + op->lval.sword);
        case 32:
           return (unsigned long)(pc + op->lval.sdword);
        case 64:
           return (unsigned long)(pc + op->lval.sqword);
        default:
           return pc;
    }
}

// flag masks
static int CF = 0x1;
static int PF = 0x4;
// static int AF = 0x10; unused right now
static int ZF = 0x40;
static int SF = 0x80;
static int OF = 0x800;

/**
 * Determines if the condition encoded in the mnemonic is met with the
 * specified context
 *
 * @param mnemonic the mnemonic
 * @param context
 *
 * @return non-zero if the condition is met; 0 otherwise
 */
static
int ctf_condition_met(ud_mnemonic_code_t mnemonic, const void *context) {
    int result = 0;

    unsigned long flags = get_flags(context);
    unsigned long cx = (__WORDSIZE == 64) ? get_register_ud_type(UD_R_RCX, context)
        : get_register_ud_type(UD_R_ECX, context);

    switch (mnemonic) {
        case UD_Ija:
            result = (!(CF & flags) && !(ZF & flags));
            break;
        case UD_Ijae:
            result = !(CF & flags);
            break;
        case UD_Ijb:
            result = CF & flags;
            break;
        case UD_Ijbe:
            result = (CF & flags) || (ZF & flags);
            break;
        case UD_Ijcxz:
            result = get_register_ud_type(UD_R_CX, context) == 0;
            break;
        case UD_Ijecxz:
            result = get_register_ud_type(UD_R_ECX, context) == 0;
            break;
        case UD_Ijrcxz:
            result = get_register_ud_type(UD_R_RCX, context) == 0;
            break;
        case UD_Ijg:
            result = (!(ZF & flags) && ((SF & flags) == (OF & flags)));
            break;
        case UD_Ijge:
            result = ((SF & flags) == (OF & flags));
            break;
        case UD_Ijl:
            result = ((SF & flags) != (OF & flags));
            break;
        case UD_Ijle:
            result = ((ZF & flags) || ((SF & flags) != (OF & flags)));
            break;
        case UD_Ijno:
            result = !(OF & flags);
            break;
        case UD_Ijnp:
            result = !(PF & flags);
            break;
        case UD_Ijns:
            result = !(SF & flags);
            break;
        case UD_Ijnz:
            result = !(ZF & flags);
            break;
        case UD_Ijo:
            result = OF & flags;
            break;
        case UD_Ijp:
            result = PF & flags;
            break;
        case UD_Ijs:
            result = SF & flags;
            break;
        case UD_Ijz:
            result = ZF & flags;
            break;
        case UD_Iloopne: 
            result = (!(ZF & flags) && (cx-1) != 0);
            break;
        case UD_Iloope:
            result = ((ZF & flags) && (cx-1) != 0);
            break;
        case UD_Iloop:
            result = (cx-1) != 0;
            break;
        default:
            break;
    }

    return result;
}

/**
 * Computes the target encoded by the operand
 *
 * @param mnemonic the mnemonic for the instruction
 * @param op the operand
 * @param pc the pc
 * @param effective_pc the effective pc (points to the next instruction)
 * @Param context the context (used to retrieve registers)
 *
 * @return the target or 0 on failure
 */
static
unsigned long compute_target(ud_mnemonic_code_t mnemonic,
                             struct ud_operand *op,
                             unsigned long pc,
                             unsigned long effective_pc,
                             const void *context)
{
    USE(mnemonic);
    USE(pc);

    switch(op->type) {
        case UD_OP_REG:
            return get_register_ud_type(op->base, context);
        case UD_OP_MEM: 
        {
            unsigned long base;
            if (op->base != UD_NONE) {
                base = get_register_ud_type(op->base, context);
            }else{
                base = 0;
            }

            unsigned long index;
            if (op->index != UD_NONE) {
                index = get_register_ud_type(op->index, context);
            }else{
                index = 0;
            }

            unsigned long displacement;
            if (op->offset != 0) {
                switch (op->offset) {
                    case 8:
                        displacement = op->lval.ubyte;
                        break;
                    case 16:
                        displacement = op->lval.uword;
                        break;
                    case 32:
                        displacement = op->lval.udword;
                        break;
                    case 64:
                        displacement = op->lval.uqword;
                        break;
                    default:
                        displacement = 0;
                        break;
                }
            }else{
                displacement = 0;
            }
            return base + (index * op->scale) + displacement;
        }
        case UD_OP_JIMM:
            return compute_relative_location(op, effective_pc);
        default:
            break;
    }

    return 0;
}

uint64_t get_ctf_successor(uint64_t pc, udi_errmsg *errmsg, const void *context) {

    ud_t ud_obj;

    ud_init(&ud_obj);

    ud_set_mode(&ud_obj, __WORDSIZE);

    ud_set_input_buffer(&ud_obj, (unsigned char *)pc, 15);

    ud_set_pc(&ud_obj, pc);

    if ( ud_disassemble(&ud_obj) == 0 ) {
        udi_set_errmsg(errmsg, "disassembling instruction at %a failed", pc);
        udi_log("%s", errmsg->msg);
        return 0;
    }

    struct ud_operand *first_op = &(ud_obj.operand[0]);

    unsigned long successor = 0;
    switch (ud_obj.mnemonic) {
        case UD_Icall:
        case UD_Ijmp:
            // unconditional control transfer
            successor = compute_target(ud_obj.mnemonic,
                                       first_op,
                                       pc,
                                       ud_obj.pc,
                                       context);
            break;
        case UD_Ijo:
        case UD_Ijno:
        case UD_Ijb:
        case UD_Ijae:
        case UD_Ijz:
        case UD_Ijnz:
        case UD_Ijbe:
        case UD_Ija:
        case UD_Ijs:
        case UD_Ijns:
        case UD_Ijp:
        case UD_Ijnp:
        case UD_Ijl:
        case UD_Ijge:
        case UD_Ijle:
        case UD_Ijg:
        case UD_Ijcxz:
        case UD_Ijecxz:
        case UD_Ijrcxz:
        case UD_Iloopne:
        case UD_Iloope:
        case UD_Iloop:
            if ( ctf_condition_met(ud_obj.mnemonic, context) ) {
                successor = compute_target(ud_obj.mnemonic,
                                           first_op,
                                           pc,
                                           ud_obj.pc,
                                           context);
            }else{
                successor = pc + ud_insn_len(&ud_obj);
            }
            break;
        case UD_Iret:
        {
            uintptr_t stack_ptr;
            if (__WORDSIZE == 64) {
                stack_ptr = get_register_ud_type(UD_R_RSP, context);
            }else{
                stack_ptr = get_register_ud_type(UD_R_ESP, context);
            }

            if ( read_memory((uint8_t *)&successor, (const uint8_t *)stack_ptr, sizeof(unsigned long),
                        errmsg) ) return 0;

            return successor;

            break;
        }
        default:
            // the easy case, just the next instruction
            successor = pc + ud_insn_len(&ud_obj);
            break;
    }

    if ( successor == 0 ) {
        udi_set_errmsg(errmsg,
                       "failed to determine ctf successor at %a",
                       pc);
        udi_log("%s", errmsg->msg);
    }

    return successor;
}

int is_gp_register(udi_register_e reg) {
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
        case UDI_X86_64_R8:
        case UDI_X86_64_R9:
        case UDI_X86_64_R10:
        case UDI_X86_64_R11:
        case UDI_X86_64_R12:
        case UDI_X86_64_R13:
        case UDI_X86_64_R14:
        case UDI_X86_64_R15:
        case UDI_X86_64_RDI:
        case UDI_X86_64_RSI:
        case UDI_X86_64_RBP:
        case UDI_X86_64_RBX:
        case UDI_X86_64_RDX:
        case UDI_X86_64_RAX:
        case UDI_X86_64_RCX:
        case UDI_X86_64_RSP:
        case UDI_X86_64_RIP:
        case UDI_X86_64_CSGSFS:
        case UDI_X86_64_FLAGS:
            return 1;
        default:
            return 0;
    }
}

int is_fp_register(udi_register_e reg) {
    switch (reg) {
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
            return 1;
        default:
            return 0;
    }
}
