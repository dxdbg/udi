/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
