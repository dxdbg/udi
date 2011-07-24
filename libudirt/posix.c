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

/* UDI implementation specific to POSIX-compliant operating systems */

#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "udirt.h"
#include "udi.h"

/* constants */
static const unsigned int ERRMSG_SIZE = 4096;
static const unsigned int PID_STR_LEN = 10;

/* file paths */
static char *basedir_name;
static char *requestfile_name;
static char *responsefile_name;

/* file handles */
static FILE *request_handle = -1;
static FILE *response_handle = -1;
static FILE *event_handle = -1;

/* platform specific variables */
const char *UDI_DS = "/";
const char *DEFAULT_UDI_ROOT_DIR = "/tmp/udi";

#define UDI_CONSTRUCTOR __attribute__((constructor))

/* wrapper function types */

typedef int (*sigaction_type)(int, const struct sigaction *, 
        struct sigaction *);

typedef pid_t (*fork_type)(void);

typedef int (*execve_type)(const char *, char *const, char *const);

/* wrapper function pointers */

sigaction_type real_sigaction;
fork_type real_fork;
execve_type real_execve;

void init_udi_rt() UDI_CONSTRUCTOR 
{
    char errmsg[ERRMSG_SIZE];
    char *errmsg_tmp;
    int errnum = 0, output_enabled = 0;

    /* initialize error message */
    errmsg[ERRMSG_SIZE-1] = '\0';

    /* turn on debugging, if necessary */
    if (getenv(UDI_DEBUG_ENV) != NULL) 
    {
        udi_debug_on = 1;
    }

    /* reset dlerror() */
    dlerror();

    do{
        /* initialize pseudo filesystem */

        /* first, get root directory */
        if ((UDI_ROOT_DIR = getenv(UDI_ROOT_DIR_ENV)) == NULL) 
        {
            UDI_ROOT_DIR = DEFAULT_UDI_ROOT_DIR;
        }

        /* create the directory for this process */
        size_t basedir_length = strlen(UDI_ROOT_DIR) + PID_STR_LEN;
        basedir_name = (char *)malloc(basedir_length);
        if (basedir_name == NULL)
        {
            udi_printf("malloc failed: %s\n", strerror());
            errnum = errno;
            break;
        }

        basedir_name[basedir_length-1] = '\0';
        snprintf(basedir_name, basedir_length-1, "%s/%d", UDI_ROOT_DIR,
                getpid());
        if (mkdir(basedir_name, S_IRWXG | S_IRWXU) == -1) 
        {
            udi_printf("error creating basedir: %s\n", strerror());
            errnum = errno;
            break;
        }

        /* create the udi files */
        size_t requestfile_length = strlen(UDI_ROOT_DIR) + PID_STR_LEN
            + strlen(REQUEST_FILE_NAME);
        requestfile_name = (char *)malloc(requestfile_length);
        if (requestfile_name == NULL)
        {
            udi_printf("malloc failed: %s\n", strerror());
            errnum = errno;
            break;
        }

        if (mkfifo(requestfile_name, S_IRWXG | S_IRWXU) == -1)
        {
            udi_printf("error creating request file fifo: %s\n", strerror());
            errnum = errno;
            break;
        }
        
        /* start the debugger handshake */
        int request_fd = -1;
        if ((request_fd = open(requestfile_name, O_NONBLOCK | O_RDONLY)) == -1 )
        {
            udi_printf("error open request file fifo: %s\n", strerror());
            errnum = errno;
            break;
        }

        if ((request_handle = fdopen(request_fd, "r")) == NULL) 
        {
            udi_printf("error creating request file stream: %s\n", strerror());
            errnum = errno;
            break;
        }

        size_t responsefile_length = strlen(UDI_ROOT_DIR) + PID_STR_LEN
            + strlen(RESPONSE_FILE_NAME);
        responsefile_name = (char *)malloc(responsefile_length);
        if (responsefile_name == NULL)
        {
            udi_printf("malloc failed: %s\n", strerror());
            errnum = errno;
            break;
        }

        if (mkfifo(responsefile_name, S_IRWXG | S_IRWXU) == -1)
        {
            udi_printf("error creating response file fifo: %s\n", strerror());
            errnum = errno;
            break;
        }

        size_t eventfile_length = strlen(UDI_ROOT_DIR) + PID_STR_LEN
            + strlen(STATE_FILE_NAME);
        eventfile_name = (char *)malloc(eventfile_length);
        if (eventfile_name == NULL)
        {
            udi_printf("malloc failed: %s\n", strerror());
            errnum = errno;
            break;
        }

        if (mkfifo(eventfile_name, S_IRWXG | S_IRWXU) == -1)
        {
            udi_printf("error creating event file fifo: %s\n", strerror());
            errnum = errno;
            break;
        }

        /* complete handshake with debugger */

        /* locate symbols for wrapper functions */
        real_sigaction = (sigaction_type) dlsym(RTLD_NEXT, "sigaction");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) 
        {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg, errmsg_tmp, ERRMSG_SIZE-1);
            errnum = -1;
            break;
        }

        real_fork = (fork_type) dlsym(RTLD_NEXT, "fork");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) 
        {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg, errmsg_tmp, ERRMSG_SIZE-1);
            errnum = -1;
            break;
        }

        real_execve = (execve_type) dlsym(RTLD_NEXT, "execve");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) 
        {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg, errmsg_tmp, ERRMSG_SIZE-1);
            errnum = -1;
            break;
        }
    }while(0);

    if (errnum > 0) 
    {
        strerror_r(errnum, errmsg, ERRMSG_SIZE-1);
    }

    if (errnum != 0) 
    {
        if(!output_enabled) 
        {
            fprintf(stderr, "Failed to initialize udi rt: %s\n", errmsg);
        }else
        {
            // TODO write a response to the events fifo
        }
    }
}

/* request-response handling */

/* brew-your-own because htonl doesn't handle 64-bit */

// TODO needs to be architecture independent -- currently for little endian
static
uint64_t udi_unpack_uint64_t(unsigned char *buffer) {
    uint64_t ret = 0;

    int i;
    for(i = 0; i < sizeof(uint64_t); ++i) {
        // TODO
    }
}

static udi_request *read_request(FILE *stream) 
{
    udi_request *request = udi_malloc(sizeof(udi_request));

    if (request == NULL) 
    {
        udi_printf("malloc failed: %s\n", strerror());
        return NULL;
    }

    /* read the request type and length */

    udi_length total = 0;
    while( total < length )
    {
        size_t bytes_read = fread(dest, ((size_t)length - total)
    }
}

udi_request *read_request()
{
    return read_request(request_handle);
}

/* wrapper function implementations */

int sigaction(int signum, const struct sigaction *act,
        struct sigaction *oldact)
{
    // TODO wrapper function stuff

    return real_sigaction(signum, act, oldact);
}

pid_t fork()
{
    // TODO wrapper function stuff

    return real_fork();
}

int execve(const char *filename, char *const argv,
        char *const envp)
{
    // TODO wrapper function stuff

    return real_execve(filename, argv, envp);
}
