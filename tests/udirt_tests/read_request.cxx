/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#ifdef UNIX
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "udirt.h"
#include "libuditest.h"

using std::cout;
using std::endl;

class test_read_request : public UDITestCase {
    public:
        test_read_request()
            : UDITestCase(std::string("test_read_request")) {}
        virtual ~test_read_request() {}
        
        bool operator()(void);
};

static test_read_request testInstance;

#ifdef UNIX
extern "C" {
extern udi_request *read_request_from_fd(int fd);
}

static const char *TMPFILEPATH = "/tmp/udi_test_tmpfile";
static const char *TESTPAYLOAD = "testing...";
static bool test_read_request_platform() {
    // First, create a dummy file to read from 
    int fd = open(TMPFILEPATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if ( fd == -1 ) {
        cout << "Failed to create temporary file " << TMPFILEPATH
             << ": " << strerror(errno) << endl;
        return false;
    }

    // Create the request
    udi_request_type request_type = udi_request_type_hton(UDI_REQ_WRITE_MEM);
    if ( write(fd, &request_type, sizeof(udi_request_type)) == -1 ) {
        cout << "Failed to write request type: " << strerror(errno) << endl;
        return false;
    }

    udi_length length = udi_length_hton(strlen(TESTPAYLOAD) + 1);
    udi_length exp_length = strlen(TESTPAYLOAD) + 1;
    if ( write(fd, &length, sizeof(udi_length)) == -1 ) {
        cout << "Failed to write length: " << strerror(errno) << endl;
        return false;
    }

    if ( write(fd, TESTPAYLOAD, exp_length) == -1 ) {
        cout << "Failed to write payload: " << strerror(errno) << endl;
        return false;
    }

    // Reset to behave like a fifo
    if ( lseek(fd, 0, SEEK_SET) == ((off_t)-1)) {
        cout << "Failed to reset file to beginning: " << strerror(errno) 
             << endl;
        return false;
    }

    // Now, actually test the function
    udi_request *result = read_request_from_fd(fd);
    if (result == NULL) {
        cout << "Failed to read request from file" << endl;
        return false;
    }

    if (result->request_type != UDI_REQ_WRITE_MEM)
    {
        cout << "Unexpected value in request ( " << UDI_REQ_WRITE_MEM
             << " != " << result->request_type << " ) " << endl;
        return false;
    }

    if ( result->length != exp_length ) {
        cout << "Unexpected value in request ( " << length
             << " != " << result->length << " ) " << endl;
        return false;
    }

    if ( std::string((char *)result->packed_data) != 
         std::string(TESTPAYLOAD) )
    {
        cout << "Unexpected value in request ( '"
             << (char *)result->packed_data
             << "' != '" << TESTPAYLOAD << "' ) " << endl;
        return false;
    }

    // Cleanup
    
    close(fd); // don't care on error

    unlink(TMPFILEPATH); // don't care on error

    return true;
}
#else
static bool test_read_request_platform() {
    return false;
}
#endif

bool test_read_request::operator()(void) {
    return test_read_request_platform();
}
