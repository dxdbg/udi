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

class test_singlestep : public UDITestCase {
    public:
        test_singlestep()
            : UDITestCase(std::string("test_singlestep")) {}
        virtual ~test_singlestep() {}

        bool operator()(void);
};

static const char *TEST_BINARY = SIMPLE_BINARY_PATH;
static udi_address TEST_FUNCTION = SIMPLE_FUNCTION2;
static unsigned long TEST_FUNCTION_LEN = SIMPLE_FUNCTION2_LENGTH;

static test_singlestep testInstance;

bool test_singlestep::operator()(void) {
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

    // Enable single stepping
    result = set_single_step(thr, 1);
    assert_no_error(proc, result);
    test_assert(get_single_step(thr) == 1);

    udi_address current_pc;
    result = get_pc(thr, &current_pc);
    assert_no_error(proc, result);
    test_assert(current_pc == TEST_FUNCTION);

    // Single step through the whole test function
    udi_address next_pc;
    do {
        result = continue_process(proc);
        assert_no_error(proc, result);

        wait_for_single_step(thr);

        result = get_next_instruction(thr, &next_pc);
        assert_no_error(proc, result);

        result = get_pc(thr, &current_pc);
        assert_no_error(proc, result);
    }while (    (current_pc < (TEST_FUNCTION + TEST_FUNCTION_LEN))
             && (next_pc < (TEST_FUNCTION + TEST_FUNCTION_LEN)) );

    // Disable single stepping
    result = set_single_step(thr, 0);
    assert_no_error(proc, result);
    test_assert(get_single_step(thr) == 0);

    result = continue_process(proc);
    assert_no_error(proc, result);

    wait_for_exit(thr, EXIT_FAILURE);

    return true;
}
