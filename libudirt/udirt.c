/*
 * Copyright (c) 2011, Dan McNulty
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
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
int failed_mem_access_response(udi_request_type request_type, char *errmsg,
        unsigned int errmsg_size)
{
    udi_response resp;
    resp.response_type = UDI_RESP_ERROR;
    resp.request_type = request_type;

    const char *errstr = get_mem_errstr();

    resp.length = strlen(errstr);
    resp.packed_data = udi_pack_data(resp.length, UDI_DATATYPE_BYTESTREAM,
        resp.length, errstr);

    int errnum = 0;
    do {
        if ( resp.packed_data == NULL ) {
            snprintf(errmsg, errmsg_size, "%s", "failed to pack response data");
            udi_printf("%s", "failed to pack response data for read request");
            errnum = -1;
            break;
        }

        errnum = write_response(&resp);
    }while(0);

    if ( resp.packed_data != NULL ) udi_free(resp.packed_data);

    return errnum;
}

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
    if ( abortable_memcpy(dest, src, num_bytes) 
            != 0 )
    {
        mem_result = failed_mem_access_response(UDI_REQ_READ_MEM, errmsg, errmsg_size);

        if ( mem_result < 0 ) {
            mem_result = REQ_ERROR;
        }
    }
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

breakpoint *create_breakpoint(udi_address breakpoint_addr) {
    breakpoint *new_breakpoint = (breakpoint *)udi_malloc(sizeof(breakpoint));

    if ( new_breakpoint == NULL ) {
        udi_printf("failed to allocate memory for breakpoint: %s\n",
                strerror(errno));
        return NULL;
    }

    new_breakpoint->saved_byte = 0;
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

int install_breakpoint(breakpoint *bp) {
    return 0;
}

int remove_breakpoint(breakpoint *bp) {
    if ( !bp->in_memory ) {
        udi_printf("breakpoint at %llx not in memory, not removed\n",
                bp->address);
        return 0;
    }

    return 0;
}

int delete_breakpoint(breakpoint *bp) {
    int errnum = remove_breakpoint(bp);

    if ( errnum ) return errnum;

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
