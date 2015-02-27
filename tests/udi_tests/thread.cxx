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
#include <set>

#include "libudi.h"
#include "libuditest.h"
#include "test_bins.h"
#include "test_lib.h"

using std::cout;
using std::endl;
using std::stringstream;
using std::string;
using std::set;
using std::pair;
using std::map;
using std::make_pair;

class test_thread : public UDITestCase {
    public:
        test_thread()
            : UDITestCase(std::string("test_thread")) {}
        virtual ~test_thread() {}

        bool operator()(void);
};

static const int NUM_THREADS = 10;

static const char *TEST_BINARY = THREAD_BINARY_PATH;
static udi_address TEST_FUNCTION = THREAD_BREAK_FUNC;

static test_thread testInstance;

/**
 * Used to process breakpoint events
 */
class BrkCallback : public EventCallback {

    public:
        BrkCallback(const set<udi_thread *> &threads)
            : threads(threads)
        {
            map<udi_thread *, udi_thread_state_e> thr_states;
            for (set<udi_thread *>::iterator i = threads.begin(); i != threads.end(); ++i) {
                thr_states.insert(make_pair(*i, UDI_TS_RUNNING));
            }
        }

        bool allEventsReceived() const { return threads.size() == 0; }

        virtual void operator()(udi_event *event);

    private:
        set<udi_thread *> threads;
        map<udi_thread *, udi_thread_state_e> thr_states;
};

void BrkCallback::operator()(udi_event *event) {
    test_assert(event->event_type == UDI_EVENT_BREAKPOINT);

    udi_event_breakpoint *brkpt = static_cast<udi_event_breakpoint *>(event->event_data);
    test_assert(brkpt->breakpoint_addr == THREAD_BREAK_FUNC);

    threads.erase(event->thr);

    validate_thread_state(thr_states);

    udi_error_e result = suspend_thread(event->thr);
    assert_no_error(event->proc, result);

    thr_states.insert(make_pair(event->thr, UDI_TS_SUSPENDED));

    validate_thread_state(thr_states);

    bool all_suspended = true;
    for (set<udi_thread *>::iterator i = threads.begin(); i != threads.end(); ++i) {
        if ( get_state(*i) == UDI_TS_RUNNING ) {
            all_suspended = false;
        }
    }

    if ( !all_suspended ) {
        result = continue_process(event->proc);
        assert_no_error(event->proc, result);
    }
}

/**
 * Used to process death events
 */
class DeathCallback : public EventCallback {
    public:
        DeathCallback(const set<udi_thread *> &threads)
            : threads(threads)
        {}

        bool allEventsReceived() const { return threads.size() == 0; }

        virtual void operator()(udi_event *event);

    private:
        set<udi_thread *> threads;
};

void DeathCallback::operator()(udi_event *event) {
    test_assert(event->event_type == UDI_EVENT_THREAD_DEATH);

    threads.erase(event->thr);

    udi_error_e result = continue_process(event->proc);
    assert_no_error(event->proc, result);
}

bool test_thread::operator()(void) {

    stringstream num_threads_str;
    num_threads_str << NUM_THREADS;

    string num_threads = num_threads_str.str();

    char *argv[] = { (char *)TEST_BINARY, (char *)num_threads.c_str(), NULL };

    udi_proc_config config;
    config.root_dir = NULL;
    
    udi_error_e error_code;
    char *errmsg = NULL;
    udi_process *proc = create_process(TEST_BINARY, argv, NULL, &config, &error_code, &errmsg);

    test_assert(proc != NULL);
    free(errmsg);
    test_assert(get_multithread_capable(proc));

    udi_thread *initial_thr = get_initial_thread(proc);
    test_assert(initial_thr != NULL);

    udi_error_e result = create_breakpoint(proc, TEST_FUNCTION);
    assert_no_error(proc, result);

    result = install_breakpoint(proc, TEST_FUNCTION);
    assert_no_error(proc, result);

    validate_thread_state(proc, UDI_TS_RUNNING);

    result = continue_process(proc);
    assert_no_error(proc, result);

    set<udi_thread *> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        udi_thread *thr = wait_for_thread_create(proc);
        test_assert(thr != NULL);

        pair<set<udi_thread *>::iterator, bool> ins_result = threads.insert(thr);
        test_assert(ins_result.second);

        validate_thread_state(proc, UDI_TS_RUNNING);

        result = continue_process(proc);
        assert_no_error(proc, result);
    }

    wait_for_debuggee_pipe(proc);

    // tell debuggee to run threads into breakpoints
    release_debuggee_threads(proc);

    // Wait for all the breakpoints to occur
    BrkCallback brkCallback(threads);
    while (!brkCallback.allEventsReceived()) {
        wait_for_events(proc, brkCallback);
    }

    for (set<udi_thread *>::iterator i = threads.begin(); i != threads.end(); ++i) {
        resume_thread(*i);
    }

    result = continue_process(proc);
    assert_no_error(proc, result);

    // tell debuggee to let threads die
    release_debuggee_threads(proc);

    DeathCallback deathCallback(threads);
    while (!deathCallback.allEventsReceived()) {
        wait_for_events(proc, deathCallback);
    }

    wait_for_exit(initial_thr, EXIT_SUCCESS);

    cleanup_debuggee_pipe(proc);

    return true;
}
