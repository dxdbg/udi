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
#include "test_lib.h"

using std::cout;
using std::endl;
using std::stringstream;

bool wait_for_breakpoint(udi_process *proc, udi_address breakpoint) {
    udi_event *events = wait_for_events(&proc, 1);

    udi_event *iter = events;

    // TODO refactor this to be shared among other functions
    bool saw_breakpoint_event = false;
    while (iter != NULL) {
        if ( iter->proc != proc ) {
            cout << "Received event for unknown process " << get_proc_pid(iter->proc) << endl;
            return false;
        }

        if ( iter->event_type != UDI_EVENT_BREAKPOINT ) {
            cout << "Received unexpected event " << get_event_type_str(iter->event_type) << endl;
            return false;
        }

        udi_event_breakpoint *proc_breakpoint = (udi_event_breakpoint *)iter->event_data;
        if ( proc_breakpoint->breakpoint_addr != breakpoint ) {
            cout << "Observed unexpected breakpoint at 0x" << std::hex
                 << proc_breakpoint->breakpoint_addr << " (expected " << breakpoint
                 << ")" << std::dec << endl;
            return false;
        }

        saw_breakpoint_event = true;

        iter = iter->next_event;
    }

    if ( events != NULL ) {
        free_event_list(events);
    }

    if ( !saw_breakpoint_event ) {
        cout << "Failed to observe breakpoint event for process " << get_proc_pid(proc) << endl;
        return false;
    }

    return true;
}

bool wait_for_exit(udi_process *proc) {
    udi_event *events = wait_for_events(&proc, 1);

    udi_event *iter = events;

    bool saw_exit_event = false;
    while ( iter != NULL ) {
        if ( iter->proc != proc ) {
            cout << "Received event for unknown process " << get_proc_pid(iter->proc) << endl;
            return false;
        }

        if ( iter->event_type != UDI_EVENT_PROCESS_EXIT ) {
            cout << "Received unexpected event " << get_event_type_str(iter->event_type) << endl;
            return false;
        }

        saw_exit_event = true;
        udi_event_process_exit *proc_exit = (udi_event_process_exit *)iter->event_data;
        if ( proc_exit->exit_code != EXIT_FAILURE ) {
            cout << "Process unexpectedly exited with " << proc_exit->exit_code << " exit code" << endl;
            return false;
        }

        iter = iter->next_event;
    }

    udi_error_e result = continue_process(proc);

    if ( result != UDI_ERROR_NONE ) {
        cout << "Failed to continue process " << get_error_message(result) << endl;
        return false;
    }

    if ( events != NULL ) {
        free_event_list(events);
    }

    if ( !saw_exit_event ) {
        cout << "Failed to observe exit event for process " << get_proc_pid(proc) << endl;
        return false;
    }

    free_process(proc);

    return true;
}
