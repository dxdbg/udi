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
#include <inttypes.h>

const int REQ_SUCCESS = 0; ///< Request processed successfully
const int REQ_ERROR = -1; ///< Unrecoverable failure caused by environment/OS error
const int REQ_FAILURE = -2; ///< Failure to process request due to invalid arguments

char *UDI_ROOT_DIR;
int udi_debug_on = 0;
int udi_enabled = 0; // not enabled until initialization complete
int udi_in_sig_handler = 0;

// read / write handling
static void *mem_access_addr = NULL;
static size_t mem_access_size = 0;

static int aborting_mem_access = 0;
static int performing_mem_access = 0;

static void *mem_abort_label = NULL;

void *get_mem_access_addr() {
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
int abortable_memcpy(void *dest, const void *src, size_t n) {
    // This should stop the compiler from messing with the label
    static void *abort_label_addr = &&abort_label;

    mem_abort_label = abort_label_addr;

    // Hopefully when compiled with optimization, this code will be vectorized if possible
    unsigned char *uc_dest = (unsigned char *)dest;
    const unsigned char *uc_src = (const unsigned char *)src;

    size_t i = 0;
    for (i = 0; i < n; ++i) {
        uc_dest[i] = uc_src[i];
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
 * @param errmsg_size the maximum size of the error message
 *
 * @return 0 on success; non-zero otherwise
 */
static
int udi_memcpy(void *dest, const void *src, size_t num_bytes,
        char *errmsg, unsigned int errmsg_size)
{
    int mem_result = 0;

    void *pre_access_result = pre_mem_access_hook();

    if ( pre_access_result == NULL ) {
        return -1;
    }

    performing_mem_access = 1;
    mem_result = abortable_memcpy(dest, src, num_bytes);
    performing_mem_access = 0;

    if ( post_mem_access_hook(pre_access_result) ) {
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
 * @param errmsg_size the maximum size of the error message
 *
 * @return 0, success; non-zero otherwise
 */
int read_memory(void *dest, const void *src, size_t num_bytes,
        char *errmsg, unsigned int errmsg_size)
{
    mem_access_addr = (void *)src;
    mem_access_size = num_bytes;

    return udi_memcpy(dest, src, num_bytes, errmsg, errmsg_size);
}

/**
 * Writes into the specified memory address using the specified
 * source.
 *
 * @param dest the destination of the write
 * @param src the source of the write
 * @param num_bytes the number of bytes to write
 * @param errmsg the error message set by this function
 * @param errmsg_size the maximum size of the error message
 */
int write_memory(void *dest, const void *src, size_t num_bytes,
        char *errmsg, unsigned int errmsg_size)
{
    mem_access_addr = dest;
    mem_access_size = num_bytes;

    return udi_memcpy(dest, src, num_bytes, errmsg, errmsg_size);
}

// breakpoint implementation

static breakpoint *breakpoints = NULL;

/**
 * Creates a breakpoint at the specified address
 *
 * @param breakpoint_addr the breakpoint address
 *
 * @return the created breakpoint, NULL on failure
 */
breakpoint *create_breakpoint(udi_address breakpoint_addr) {
    breakpoint *new_breakpoint = (breakpoint *)udi_malloc(sizeof(breakpoint));

    if ( new_breakpoint == NULL ) {
        udi_printf("failed to allocate memory for breakpoint: %s\n",
                strerror(errno));
        return NULL;
    }

    memset(new_breakpoint->saved_bytes, 0, sizeof(new_breakpoint->saved_bytes));
    new_breakpoint->address = breakpoint_addr;
    new_breakpoint->in_memory = 0;

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

/**
 * Installs the breakpoint into memory
 *
 * @param bp the breakpoint to install
 * @param errmsg the errmsg populated by the memory access
 * @param errmsg_size the maximum size of the error message
 *
 * @return 0 on success; non-zero otherwise
 */
int install_breakpoint(breakpoint *bp, char *errmsg, unsigned int errmsg_size) {
    if ( bp->in_memory ) return 0;

    int result = write_breakpoint_instruction(bp, errmsg, errmsg_size);

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
 * @param errmsg_size the maximum size of the error message
 *
 * @return 0 on success; non-zero otherwise
 */
int remove_breakpoint(breakpoint *bp, char *errmsg, unsigned int errmsg_size) {
    if ( !bp->in_memory ) return 0;

    int result = write_saved_bytes(bp, errmsg, errmsg_size);

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
 * @param errmsg_size the maximum size of the error message
 *
 * @return 0 on success; non-zero otherwise
 */
int remove_breakpoint_for_continue(breakpoint *bp, char *errmsg,
        unsigned int errmsg_size) {

    if ( !bp->in_memory ) return 0;

    // Don't set the in memory flag
    return write_saved_bytes(bp, errmsg, errmsg_size);
}

/**
 * Deletes the breakpoint
 *
 * @param bp the breakpoint to delete
 * @param errmsg the errmsg populated by the memory access
 * @param errmsg_size the maximum size of the error message
 *
 * @return 0 on success; non-zero otherwise
 */
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
        snprintf(errmsg, errmsg_size, "failed to delete breakpoint at %"PRIx64,
                bp->address);
        udi_printf("%s\n", errmsg);
        return -1;
    }

    if (tmp_breakpoint == breakpoints) {
        breakpoints = NULL;
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
breakpoint *find_breakpoint(udi_address breakpoint_addr) {
    breakpoint *tmp_breakpoint = breakpoints;

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
