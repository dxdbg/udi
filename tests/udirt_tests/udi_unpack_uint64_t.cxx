/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cstdlib>

#include "udirt.h"
#include "libuditest.h"

extern "C" {
extern uint64_t udi_unpack_uint64_t(uint64_t value);
extern uint32_t udi_unpack_uint32_t(uint32_t value);
extern uint16_t udi_unpack_uint16_t(uint16_t value);
}

class test_udi_unpack_uint64_t : public UDITestCase {
    public:
        test_udi_unpack_uint64_t()
            : UDITestCase(std::string("test_udi_unpack_uint64_t")) {}
        virtual ~test_udi_unpack_uint64_t() {}

        bool operator()(void);
};

static test_udi_unpack_uint64_t testInstance;

bool test_udi_unpack_uint64_t::operator()(void) {
    uint64_t tmp = 0x1234567800aabbccLL;
    uint64_t expected = 0xccbbaa0078563412LL;

    if ( udi_unpack_uint64_t(tmp) != expected ) {
        return false;
    }

    uint32_t tmp32 = 0x12345678;
    uint32_t tmp32_expected = 0x78563412;

    if ( udi_unpack_uint32_t(tmp32) != tmp32_expected ) {
        return false;
    }

    uint16_t tmp16 = 0x1234;
    uint16_t tmp16_expected = 0x3412;

    if ( udi_unpack_uint16_t(tmp16) != tmp16_expected ) {
        return false;
    }

    return true;
}
