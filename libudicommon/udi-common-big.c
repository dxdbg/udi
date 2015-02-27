/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public                              
 * License, v. 2.0. If a copy of the MPL was not distributed with this                              
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
  */

// big endian specific functions

#include <stdint.h>

// big endian is network byte order

uint64_t udi_unpack_uint64_t(uint64_t value) {
    return value;
}

uint32_t udi_unpack_uint32_t(uint32_t value) {
    return value;
}

uint16_t udi_unpack_uint16_t(uint16_t value) {
    return value;
}

int is_big_endian() {
    return 1;
}
