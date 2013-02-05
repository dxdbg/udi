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
#include <cstring>
#include <iostream>
#include <stdint.h>

#include "udi-common.h"
#include "libuditest.h"

using std::cout;
using std::endl;

class test_pack_data : public UDITestCase {
    public:
        test_pack_data()
            : UDITestCase(std::string("test_pack_data")) {}
        virtual ~test_pack_data() {}

        bool operator()(void);
};

static test_pack_data testInstance;

bool test_pack_data::operator()(void) {
    uint32_t one = 1;
    uint16_t two = 2;
    uint64_t three = 3;
    const char *test = "TEST";

    udi_length length = sizeof(one) + sizeof(two) + 
        sizeof(three) + strlen(test) + 1 + sizeof(udi_length);

    void *data = udi_pack_data(length,
                UDI_DATATYPE_INT32, one, UDI_DATATYPE_INT16, two,
                UDI_DATATYPE_INT64, three, UDI_DATATYPE_BYTESTREAM,
                strlen(test) + 1, test);
    if ( data == NULL ) {
        cout << "Failed to pack data" << endl;
        return false;
    }

    uint32_t one_packed;
    uint16_t two_packed;
    uint64_t three_packed;
    char *test_packed;
    udi_length str_length;

    int result = udi_unpack_data(data, length, UDI_DATATYPE_INT32,
            &one_packed, UDI_DATATYPE_INT16, &two_packed, UDI_DATATYPE_INT64,
            &three_packed, UDI_DATATYPE_BYTESTREAM, &str_length, &test_packed);

    if ( result ) {
        cout << "Failed to unpack data" << endl;
        return false;
    }

    if ( one != one_packed ) {
        cout << "Invalid value for packed integer ( " << one
             << " != " << std::hex << one_packed << std::dec << " )" << endl;
        return false;
    }

    if ( two != two_packed ) {
        cout << "Invalid value for packed short ( " << two
             << " != " << two_packed << " ) " << endl;
        return false;
    }

    if ( three != three_packed ) {
        cout << "Invalid value for packed long ( " << three
             << " != " << three_packed << " ) " << endl;
        return false;
    }

    if ( strncmp(test, test_packed, 5) != 0 ) {
        cout << "Invalid value for packed string ( " << test
             << " != " << test_packed << " ) " << endl;
        return false;
    }

    free(test_packed);
    free(data);

    return true;
}
