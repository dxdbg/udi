/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
