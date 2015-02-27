/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_lib.h"
#include "libudi.h"
#include "libuditest.h"

#include <sstream>
#include <cerrno>
#include <cstring>
#include <map>

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using std::stringstream;
using std::string;
using std::map;

static const string PIPE_BASE_NAME("/tmp/waitthread-");

static map<udi_process *, int> pipe_fds;

/**
 * Releases the debuggee threads via the pipe
 *
 * @param proc the process
 */
void release_debuggee_threads(udi_process *proc) {
    map<udi_process *, int>::iterator iter = pipe_fds.find(proc);
    test_assert( iter != pipe_fds.end());

    int pipe_fd = iter->second;

    char character = 'e';

    if ( write(pipe_fd, &character, sizeof(char)) == -1 ) {
        perror("write");
        test_assert(false);
    }

}

static
string get_pipe_name(udi_process *proc) {
    stringstream pipe_name_stream;

    pipe_name_stream << PIPE_BASE_NAME << get_proc_pid(proc);

    return pipe_name_stream.str();
}

/**
 * Cleans up the resources for the debuggee pipe
 *
 * @param proc the process
 */
void cleanup_debuggee_pipe(udi_process *proc) {
    map<udi_process *, int>::iterator iter = pipe_fds.find(proc);
    test_assert( iter != pipe_fds.end());

    int pipe_fd = iter->second;

    // Explicitly ignore errors
    close(pipe_fd);
}

/**
 * Waits for debuggee pipe to be created
 *
 * @param proc the process
 */
void wait_for_debuggee_pipe(udi_process *proc) {

    string pipe_name = get_pipe_name(proc);

    struct stat pipe_stat;

    // Wait for the pipe to exist
    while(1) {
        if ( stat(pipe_name.c_str(), &pipe_stat) == 0 ) {
            break;
        }else{
            if (errno != ENOENT ) {
                perror("stat");
                test_assert(false);
            }
        }
    }

    int pipe_fd = open(pipe_name.c_str(), O_WRONLY);
    if (pipe_fd == -1) {
        perror("open");
        test_assert(false);
    }

    pipe_fds[proc] = pipe_fd;
}
