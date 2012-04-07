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

// UDI RT implementation specific to x86

#include "udirt.h"
#include "udirt-x86.h"

static const unsigned char BREAKPOINT_INSN = 0xcc;

/**
 * Writes the breakpoint instruction for the specified breakpoint
 *
 * @param bp the breakpoint
 * @param errmsg the errmsg populated by the memory access
 * @param errmsg_size the maximum size of the error message
 *
 * @return non-zero on failure
 */
int write_breakpoint_instruction(breakpoint *bp, char *errmsg, unsigned int errmsg_size) {
    if ( bp->in_memory ) return 0;

    int result = read_memory(bp->saved_bytes, (void *)(unsigned long)bp->address, sizeof(BREAKPOINT_INSN),
            errmsg, errmsg_size);
    if( result != 0 ) {
        udi_printf("failed to save original bytes at 0x%llx\n",
                bp->address);
        return result;
    }

    result = write_memory((void *)(unsigned long)bp->address, &BREAKPOINT_INSN, sizeof(BREAKPOINT_INSN),
            errmsg, errmsg_size);
    if ( result != 0 ) {
        udi_printf("failed to install breakpoint at 0x%llx\n",
                bp->address);
    }

    return result;
}

/**
 * Writes the saved bytes for the breakpoint into memory
 *
 * @param bp the breakpoint that stores the saved bytes
 * @param errmsg the error message populated by the memory access
 * @param errmsg_size the maximum size of the error message
 *
 * @return non-zero on failure
 */
int write_saved_bytes(breakpoint *bp, char *errmsg, unsigned int errmsg_size) {
    if ( !bp->in_memory ) return 0;

    int result = write_memory((void *)(unsigned long)bp->address, bp->saved_bytes, sizeof(BREAKPOINT_INSN),
            errmsg, errmsg_size);

    if ( result != 0 ) {
        udi_printf("failed to remove breakpoint at 0x%llx\n", bp->address);
    }

    return result;
}

/**
 * Gets the architecture of this process
 *
 * @return the architecture enum
 */
udi_arch_e get_architecture() {
    return UDI_ARCH_X86;
}
