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

// UDI message payload handling

#include "udi-common.h"

#include <string.h>

/**
 * Creates an error event
 *
 * @param errmsg the error message
 * @param errmsg_size the maximum size for the error message
 *
 * @return the created event
 */
udi_event_internal create_event_error(const char *errmsg, unsigned int errmsg_size) {
    udi_length payload_length = strnlen(errmsg, errmsg_size) + 1;

    udi_event_internal result;
    result.event_type = UDI_EVENT_ERROR;
    result.length = sizeof(udi_length) + payload_length;
    result.packed_data = udi_pack_data(result.length,
            UDI_DATATYPE_BYTESTREAM, payload_length, errmsg);

    return result;
}

/**
 * Creates an unknown event
 *
 * @return the created event
 */
udi_event_internal create_event_unknown() {
    udi_event_internal result;
    result.event_type = UDI_EVENT_UNKNOWN;
    result.length = 0;
    result.packed_data = NULL;

    return result;
}
