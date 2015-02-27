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
#include <sstream>

#include "libudi.h"
#include "libuditest.h"
#include "test_bins.h"
#include "test_lib.h"

using std::cout;
using std::endl;
using std::stringstream;
using std::string;

class test_breakpoint : public UDITestCase {
    public:
        test_breakpoint()
            : UDITestCase(std::string("test_breakpoint")) {}
        virtual ~test_breakpoint() {}

        bool operator()(void);
};

static const char *TEST_BINARY = SIMPLE_BINARY_PATH;
static udi_address TEST_FUNCTION = SIMPLE_FUNCTION1;

static test_breakpoint testInstance;

bool test_breakpoint::operator()(void) {
    char *argv[] = { NULL };

    udi_proc_config config;
    config.root_dir = NULL;

    udi_error_e error_code;
    char *errmsg = NULL;
    udi_process *proc = create_process(TEST_BINARY, argv, NULL, &config, &error_code, &errmsg);
    free(errmsg);

    test_assert(proc != NULL);

    udi_thread *thr = get_initial_thread(proc);
    test_assert(thr != NULL);

    udi_error_e result = create_breakpoint(proc, TEST_FUNCTION);
    assert_no_error(proc, result);

    result = install_breakpoint(proc, TEST_FUNCTION);
    assert_no_error(proc, result);

    result = continue_process(proc);
    assert_no_error(proc, result);

    wait_for_breakpoint(thr, TEST_FUNCTION);

    udi_address pc;
    result = get_pc(thr, &pc);
    assert_no_error(proc, result);

    stringstream msg;
    msg << "Actual: "
        << std::hex
        << pc
        << std::dec
        << " Expected: "
        << std::hex
        << TEST_FUNCTION
        << std::dec;
    test_assert_msg(msg.str().c_str(), pc == TEST_FUNCTION);

    result = continue_process(proc);
    assert_no_error(proc, result);

    wait_for_exit(thr, EXIT_FAILURE);

    return true;
}
