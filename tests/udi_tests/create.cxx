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

class test_create : public UDITestCase {
    public:
        test_create()
            : UDITestCase(std::string("test_create")) {}
        virtual ~test_create() {}

        bool operator()(void);
};

static const char *TEST_BINARY = SIMPLE_BINARY_PATH;

static test_create testInstance;

bool test_create::operator()(void) {
    char *argv[] = { NULL };

    udi_proc_config config;
#if UNIX
    config.root_dir = "/tmp/create-test-udi"; // test a non-default root directory
#else
    config.root_dir = NULL;
#endif

    udi_error_e error_code;
    char *errmsg = NULL;
    udi_process *proc = create_process(TEST_BINARY, argv, NULL, &config, &error_code, &errmsg);

    test_assert(proc != NULL);
    free(errmsg);

    test_assert(get_multithread_capable(proc) == 0);

    udi_thread *thr = get_initial_thread(proc);
    test_assert(thr != NULL);
    
    udi_error_e result = continue_process(proc);
    assert_no_error(proc, result);

    wait_for_exit(thr, EXIT_FAILURE);

    return true;
}
