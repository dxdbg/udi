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
#include <map>

#include "libudi.h"
#include "libuditest.h"
#include "test_bins.h"
#include "test_lib.h"

using std::cout;
using std::endl;
using std::stringstream;
using std::map;
using std::make_pair;
using std::string;

class test_thread : public UDITestCase {
    public:
        test_thread()
            : UDITestCase(std::string("test_thread")) {}
        virtual ~test_thread() {}

        bool operator()(void);
};

static const int NUM_THREADS = 2;

static const char *TEST_BINARY = THREAD_BINARY_PATH;
static udi_address TEST_FUNCTION = THREAD_BREAK_FUNC;

static test_thread testInstance;

bool test_thread::operator()(void) {

    udi_thread * threads[NUM_THREADS];

    udi_error_e result = init_libudi();
    assert_no_error(result);

    stringstream num_threads_str;
    num_threads_str << NUM_THREADS;

    string num_threads = num_threads_str.str();

    char *argv[] = { (char *)TEST_BINARY, (char *)num_threads.c_str(), NULL };

    udi_process *proc = create_process(TEST_BINARY, argv, NULL);
    test_assert(proc != NULL);
    test_assert(get_multithread_capable(proc));

    udi_thread *initial_thr = get_initial_thread(proc);
    test_assert(initial_thr != NULL);

    result = create_breakpoint(proc, TEST_FUNCTION);
    assert_no_error(result);

    result = install_breakpoint(proc, TEST_FUNCTION);
    assert_no_error(result);

    result = continue_process(proc);
    assert_no_error(result);

    map<udi_thread *, udi_event_type> thread_brk_events;
    map<udi_thread *, udi_event_type> thread_death_events;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads[i] = wait_for_thread_create(proc);

        // Ensure duplicate create events are not being created
        for (int j = i-1; j >= 0; --j) {
            test_assert(threads[i] != threads[j]);
        }

        thread_brk_events.insert(make_pair(threads[i], UDI_EVENT_BREAKPOINT));
        thread_death_events.insert(make_pair(threads[i], UDI_EVENT_THREAD_DEATH));

        result = continue_process(proc);
        assert_no_error(result);
    }

    wait_for_debuggee_pipe(proc);

    // tell debuggee to run threads into breakpoints
    release_debuggee_threads(proc);

    // Wait for all the breakpoints to occur
    map<udi_process *, map<udi_thread *, udi_event_type> > proc_events;
    proc_events.insert(make_pair(proc, thread_brk_events));

    wait_for_events(proc_events);

    // tell debuggee to let threads die
    release_debuggee_threads(proc);

    proc_events.clear();
    proc_events.insert(make_pair(proc, thread_death_events));

    wait_for_events(proc_events);

    wait_for_exit(initial_thr, EXIT_SUCCESS);

    cleanup_debuggee_pipe(proc);

    return true;
}
