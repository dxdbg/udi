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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "libudi.h"
#include "libuditest.h"
#include "test_bins.h"
#include "test_lib.h"

using std::cout;
using std::endl;
using std::stringstream;

class test_breakpoint : public UDITestCase {
    public:
        test_breakpoint()
            : UDITestCase(std::string("test_breakpoint")) {}
        virtual ~test_breakpoint() {}

        bool operator()(void);
};

static const char *TEST_BINARY = SIMPLE_BINARY_PATH;
static udi_address TEST_FUNCTION = SIMPLE_FUNCTION1;
static udi_length TEST_FUNCTION_INST = SIMPLE_FUNCTION1_INST_LENGTH;

static test_breakpoint testInstance;

bool test_breakpoint::operator()(void) {
    if ( init_libudi() != 0 ) {
        cout << "Failed to initialize libudi" << endl;
        return false;
    }

    char *argv[] = { NULL };

    udi_process *proc = create_process(TEST_BINARY, argv, NULL);
    if ( proc == NULL ) {
        cout << "Failed to create process" << endl;
        return false;
    }

    // Create and install breakpoint
    udi_error_e result = create_breakpoint(proc, TEST_FUNCTION, TEST_FUNCTION_INST);

    if ( result != UDI_ERROR_NONE ) {
        cout << "Failed to create breakpoint " << get_error_message(result) << endl;
        return false;
    }

    result = install_breakpoint(proc, TEST_FUNCTION);

    if ( result != UDI_ERROR_NONE ) {
        cout << "Failed to install breakpoint: " << get_error_message(result) << endl;
        return false;
    }

    result = continue_process(proc);

    if ( result != UDI_ERROR_NONE ) {
        cout << "Failed to continue process: " << get_error_message(result) << endl;
        return false;
    }

    if ( !wait_for_breakpoint(proc, TEST_FUNCTION) ) {
        cout << "Failed to wait for breakpoint at 0x" << std::hex
             << TEST_FUNCTION << std::dec << endl;
        return false;
    }

    result = continue_process(proc);

    if ( result != UDI_ERROR_NONE ) {
        cout << "Failed to continue process: " << get_error_message(result) << endl;
        return false;
    }

    return wait_for_exit(proc);
}
