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

#include "test_lib.h"
#include "libudi.h"

#include <sstream>
#include <cerrno>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using std::stringstream;
using std::string;

static const string PIPE_BASE_NAME("/tmp/waitthread-");

bool release_debuggee_threads(udi_process *proc) {
    stringstream pipe_name;

    pipe_name << PIPE_BASE_NAME << get_proc_pid(proc);

    int pipe_fd = open(pipe_name.str().c_str(), O_WRONLY);
    if (pipe_fd == -1) {
        perror("open");
        return false;
    }

    char character = 'e';

    if ( write(pipe_fd, &character, sizeof(char)) == -1 ) {
        perror("write");
        return false;
    }

    // Explicitly ignore errors
    close(pipe_fd);

    return true;
}


bool wait_for_debuggee_pipe(udi_process *proc) {

    stringstream pipe_name;

    pipe_name << PIPE_BASE_NAME << get_proc_pid(proc);

    struct stat pipe_stat;

    // Wait for the pipe to exist
    while(1) {
        if ( stat(pipe_name.str().c_str(), &pipe_stat) == 0 ) {
            break;
        }else{
            if (errno != ENOENT ) {
                perror("stat");
                return false;
            }
        }
    }

    return true;
}
