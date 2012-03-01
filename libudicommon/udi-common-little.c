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

// little endian specific functions

#include <stdint.h>

// inline assembly is quicker, but this is cross-compiler

uint64_t udi_unpack_uint64_t(uint64_t value) {
    uint64_t ret = 0;

    unsigned char *ret_ptr = (unsigned char *)&ret;
    unsigned char *val_ptr = (unsigned char *)&value;

    ret_ptr[0] = val_ptr[7];
    ret_ptr[1] = val_ptr[6];
    ret_ptr[2] = val_ptr[5];
    ret_ptr[3] = val_ptr[4];
    ret_ptr[4] = val_ptr[3];
    ret_ptr[5] = val_ptr[2];
    ret_ptr[6] = val_ptr[1];
    ret_ptr[7] = val_ptr[0];

    return ret;
}

uint32_t udi_unpack_uint32_t(uint32_t value) {
    uint32_t ret = 0;

    unsigned char *ret_ptr = (unsigned char *)&ret;
    unsigned char *val_ptr = (unsigned char *)&value;
    ret_ptr[0] = val_ptr[3];
    ret_ptr[1] = val_ptr[2];
    ret_ptr[2] = val_ptr[1];
    ret_ptr[3] = val_ptr[0];

    return ret;
}

uint16_t udi_unpack_uint16_t(uint16_t value) {
    uint16_t ret = 0;

    unsigned char *ret_ptr = (unsigned char *)&ret;
    unsigned char *val_ptr = (unsigned char *)&value;
    ret_ptr[0] = val_ptr[1];
    ret_ptr[1] = val_ptr[0];

    return ret;
}


int is_big_endian() {
    return 0;
}
