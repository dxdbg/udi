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

// UDI debuggee implementation common between all platforms

#include "udirt.h"

#include <string.h>
#include <errno.h>

const int REQ_SUCCESS = 0;
const int REQ_ERROR = -1;
const int REQ_FAILURE = -2;

char *UDI_ROOT_DIR;
int udi_debug_on = 0;
int udi_enabled = 0; // not enabled until initialization complete
int udi_in_sig_handler = 0;

// read / write handling
void *mem_access_addr = NULL;
size_t mem_access_size = 0;

int abort_mem_access = 0;
int performing_mem_access = 0;

static
int abortable_memcpy(void *dest, const void *src, size_t n) {
    /* slow as molasses, but gets the job done */
    unsigned char *uc_dest = (unsigned char *)dest;
    const unsigned char *uc_src = (const unsigned char *)src;

    size_t i = 0;
    for (i = 0; i < n && !abort_mem_access; ++i) {
        uc_dest[i] = uc_src[i];
    }

    if ( abort_mem_access ) {
        abort_mem_access = 0;
        return -1;
    }

    return 0;
}

static
int udi_memcpy(void *dest, const void *src, size_t num_bytes,
        char *errmsg, unsigned int errmsg_size)
{
    int mem_result = 0;

    performing_mem_access = 1;
    mem_result = abortable_memcpy(dest, src, num_bytes);
    performing_mem_access = 0;

    return mem_result;
}

int read_memory(void *dest, const void *src, size_t num_bytes,
        char *errmsg, unsigned int errmsg_size)
{
    mem_access_addr = (void *)src;
    mem_access_size = num_bytes;

    return udi_memcpy(dest, src, num_bytes, errmsg, errmsg_size);
}

int write_memory(void *dest, const void *src, size_t num_bytes,
        char *errmsg, unsigned int errmsg_size)
{
    mem_access_addr = dest;
    mem_access_size = num_bytes;

    return udi_memcpy(dest, src, num_bytes, errmsg, errmsg_size);
}

// breakpoint implementation

static breakpoint *breakpoints = NULL;

breakpoint *create_breakpoint(udi_address breakpoint_addr, udi_length instruction_length) {
    if ( instruction_length <= 0 ) {
        udi_printf("invalid argument: instruction_length: %d\n", instruction_length);
        return NULL;
    }

    breakpoint *new_breakpoint = (breakpoint *)udi_malloc(sizeof(breakpoint));

    if ( new_breakpoint == NULL ) {
        udi_printf("failed to allocate memory for breakpoint: %s\n",
                strerror(errno));
        return NULL;
    }

    memset(new_breakpoint->saved_bytes, 0, sizeof(new_breakpoint->saved_bytes));
    new_breakpoint->address = breakpoint_addr;
    new_breakpoint->in_memory = 0;
    new_breakpoint->instruction_length = instruction_length;

    // Add the breakpoint to the end of the linked list
    if ( breakpoints == NULL ) {
        breakpoints = new_breakpoint;
        new_breakpoint->next_breakpoint = NULL;
    }else{
        breakpoint *tmp_breakpoint = breakpoints;
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

int install_breakpoint(breakpoint *bp, char *errmsg, unsigned int errmsg_size) {
    if ( bp->in_memory ) {
        snprintf(errmsg, errmsg_size, "breakpoint at %llx already in memory, not installed",
                bp->address);
        udi_printf("%s\n", errmsg);
        return 0;
    }

    int result = write_breakpoint_instruction(bp, errmsg, errmsg_size);

    if ( result == 0 ) {
        bp->in_memory = 1;
    }

    return result;
}

int remove_breakpoint(breakpoint *bp, char *errmsg, unsigned int errmsg_size) {
    if ( !bp->in_memory ) {
        snprintf(errmsg, errmsg_size, "breakpoint at %llx already removed from memory",
                bp->address);
        udi_printf("%s\n", errmsg);
        return 0;
    }

    int result = write_saved_bytes(bp, errmsg, errmsg_size);

    if ( result == 0 ) {
        bp->in_memory = 0;
    }
    
    return result;
}

int delete_breakpoint(breakpoint *bp, char *errmsg, unsigned int errmsg_size) {
    int remove_result = remove_breakpoint(bp, errmsg, errmsg_size);

    if ( remove_result ) return remove_result;

    breakpoint *tmp_breakpoint = breakpoints;
    breakpoint *prev_breakpoint = breakpoints;

    while ( tmp_breakpoint != NULL ) {
        if ( tmp_breakpoint->address == bp->address ) break;
        prev_breakpoint = tmp_breakpoint;
        tmp_breakpoint = prev_breakpoint->next_breakpoint;
    }

    if ( tmp_breakpoint == NULL || prev_breakpoint == NULL ) {
        snprintf(errmsg, errmsg_size, "failed to delete breakpoint at %llx",
                bp->address);
        udi_printf("%s\n", errmsg);
        return -1;
    }

    prev_breakpoint->next_breakpoint = NULL;
    udi_free(tmp_breakpoint);

    return 0;
}

breakpoint *find_breakpoint(udi_address breakpoint_addr) {
    breakpoint *tmp_breakpoint = breakpoints;

    while ( tmp_breakpoint != NULL ) {
        if ( tmp_breakpoint->address == breakpoint_addr ) break;
        tmp_breakpoint = tmp_breakpoint->next_breakpoint;
    }

    return tmp_breakpoint;
}
