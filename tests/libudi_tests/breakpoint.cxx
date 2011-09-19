/*
 * Copyright (c) 2011, Dan McNulty
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
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

static test_breakpoint testInstance;

const char *binary_root_dir = BINARY_DIR;

const udi_address brkpt_addr = 0x8049644;

bool test_breakpoint::operator()(void) {
    stringstream testfile;
    testfile << binary_root_dir << "/breakpoint_bin";

    if ( init_libudi() != 0 ) {
        cout << "Failed to initialize libudi" << endl;
        return false;
    }

    char *argv[] = { NULL };
    char *envp[] = { NULL };

    udi_process *proc = create_process(testfile.str().c_str(), argv, envp);
    if ( proc == NULL ) {
        cout << "Failed to create process" << endl;
        return false;
    }

    udi_error_e result = set_breakpoint(proc, brkpt_addr);

    if ( result != UDI_ERROR_NONE ) {
        cout << "Failed to set breakpoint: " 
             << get_error_message(result) << endl;
        return false;
    }

    result = continue_process(proc);

    if ( result != UDI_ERROR_NONE ) {
        cout << "Failed to continue process: " 
             << get_error_message(result) << endl;
        return false;
    }

    udi_process **procs = (udi_process **)malloc(sizeof(udi_process *));
    procs[0] = proc;

    udi_event *event_list = wait_for_events(procs, 1);
    
    udi_event *current_event = event_list;
    while (current_event != NULL) {
        if ( current_event->event_type != UDI_EVENT_BREAKPOINT ) {
            cout << "Received unexpected event( " 
                 << get_event_type_str(current_event->event_type)
                 << ")" << endl;
        }else{
            udi_event_breakpoint *brkpt = 
                (udi_event_breakpoint *)current_event->event_data;

            if ( brkpt->breakpoint_addr != brkpt_addr ) {
                cout << "Unexpected breakpoint addr 0x"
                     << std::hex << brkpt->breakpoint_addr << std::dec
                     << ", expected "
                     << std::hex << brkpt_addr << std::dec
                     << endl;
                return false;
            }
        }
        current_event = current_event->next_event;
    }

    free_event_list(event_list);
    free(procs);

    return true;
}
