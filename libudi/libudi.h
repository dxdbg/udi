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

#ifndef _LIBUDI_H
#define _LIBUDI_H 1

#include "udi.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct udi_process_struct udi_process;

/**
 * library error codes
 */
typedef enum {
    UDI_ERROR_LIBRARY,
    UDI_ERROR_REQUEST,
    UDI_ERROR_NONE
} udi_error_e;

/**
 * Get a message describing the passed error code
 *
 * Note: this may be dependent on the result of the most recent operation
 *
 * @param error_code the error code
 */
const char *get_error_message(udi_error_e error_code);

/**
 * Initializes the library
 *
 * @return zero on success, non-zero on success
 */
int init_libudi();

/**
 * Sets the directory to be used for the root of the UDI filesystem
 * 
 * Will be created if it doesn't exist
 * 
 * Cannot be set if processes already created
 *
 * @param root_dir the directory to set as the root
 * @return zero on success, non-zero on success
 */
int set_udi_root_dir(const char *root_dir);

/**
 * Create UDI-controlled process
 * 
 * @param executable   the full path to the executable
 * @param argv         the arguments
 * @param envp         the environment
 *
 * @return a handle to the created process
 *
 * @see execve
 */
udi_process *create_process(const char *executable, char * const argv[],
        char * const envp[]);

/* Memory access interface */

/**
 * Access unsigned long
 *
 * @param proc          the process handle
 * @param write         if non-zero, write to specified address
 * @param value         pointer to the value to read/write
 * @param size          the size of the data block pointed to by value
 * @param addr          the location in memory to read/write
 *
 * @return non-zero on failure, 0 on success
 */
udi_error_e mem_access(udi_process *proc, int write, void *value, 
        udi_length size, udi_address addr);

/**
 * Set a breakpoint in the specified process at the specified address
 *
 * @param proc                  the process handle
 * @param breakpoint_addr       the breakpoint address
 *
 * @return non-zero on failure, 0 on success
 */
udi_error_e set_breakpoint(udi_process *proc, udi_address breakpoint_addr);

#ifdef __cplusplus
} // "C"
#endif

#endif
