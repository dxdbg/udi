/*
 * Copyright (c) 2011-2013, Dan McNulty
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
