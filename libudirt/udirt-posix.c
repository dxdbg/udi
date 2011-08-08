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

// UDI implementation specific to POSIX-compliant operating systems

////////////////////////////////////
// Includes
////////////////////////////////////

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "udirt.h"
#include "udi.h"
#include "udi-common.h"
#include "udi-common-posix.h"

////////////////////////////////////
// Definitions
////////////////////////////////////

// macros
//#define UDI_CONSTRUCTOR __attribute__((constructor))
#define UDI_CONSTRUCTOR
#define CASE_TO_STR(x) case x: return #x

// constants
static const unsigned int ERRMSG_SIZE = 4096;
static const unsigned int PID_STR_LEN = 10;

// file paths
static char *basedir_name;
static char *requestfile_name;
static char *responsefile_name;
static char *eventsfile_name;

// file handles
static int request_handle = -1;
static int response_handle = -1;
static int events_handle = -1;

// wrapper function types

typedef int (*sigaction_type)(int, const struct sigaction *, 
        struct sigaction *);

typedef pid_t (*fork_type)(void);

typedef int (*execve_type)(const char *, char *const *, char *const *);

// wrapper function pointers

sigaction_type real_sigaction;
fork_type real_fork;
execve_type real_execve;

// request handling
typedef int (*request_handler)(udi_request *, char *, unsigned int);

int continue_handler(udi_request *req, char *errmsg, unsigned int errmsg_size);
int read_handler(udi_request *req, char *errmsg, unsigned int errmsg_size);
int write_handler(udi_request *req, char *errmsg, unsigned int errmsg_size);
int state_handler(udi_request *req, char *errmsg, unsigned int errmsg_size);
int init_handler(udi_request *req, char *errmsg, unsigned int errmsg_size);

static
request_handler req_handlers[] = {
    continue_handler,
    read_handler,
    write_handler,
    state_handler,
    init_handler
};

////////////////////////////////////
// Functions
////////////////////////////////////

// wrapper function implementations

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

int execve(const char *filename, char *const argv[],
        char *const envp[])
{
    // TODO wrapper function stuff

    return real_execve(filename, argv, envp);
}

// library memory allocation -- wrappers for now

void udi_free(void *ptr) {
    free(ptr);
}

void *udi_malloc(udi_length length) {
    return malloc(length);
}

// request-response handling

void free_request(udi_request *request) {
    if ( request->argument != NULL ) udi_free(request->argument);
    udi_free(request);
}

udi_request *read_request_from_fd(int fd) {
    // easier to test if broken out into a parameterized function

    int errnum = 0;
    udi_request *request = (udi_request *)udi_malloc(sizeof(udi_request));

    if (request == NULL) 
    {
        udi_printf("malloc failed: %s\n", strerror(errno));
        return NULL;
    }

    request->argument = NULL;
    do {
        // read the request type and length
        if ( (errnum = read_all(fd, &(request->request_type),
                        sizeof(udi_request_type))) != 0 ) break;
        request->request_type = udi_unpack_uint64_t(request->request_type);

        if ( (errnum = read_all(fd, &(request->length),
                    sizeof(udi_length))) != 0) break;
        request->length = udi_unpack_uint64_t(request->length);

        // read the payload
        request->argument = udi_malloc(request->length);
        if ( (errnum = read_all(fd, request->argument,
                    request->length)) != 0) break;
    }while(0);

    if (errnum != 0) {
        udi_printf("read_all failed: %s\n", strerror(errnum));
        free_request(request);
        return NULL;
    }

    return request;
}

udi_request *read_request()
{
    return read_request_from_fd(request_handle);
}

int write_response_to_fd(int fd, udi_response *response) {
    int errnum = 0;
    do {
        udi_response_type tmp_type = response->response_type;
        tmp_type = udi_unpack_uint64_t(tmp_type);
        if ( (errnum = write_all(response_handle, &tmp_type,
                       sizeof(udi_response_type))) != 0 ) break;

        udi_request_type tmp_req_type = response->request_type;
        tmp_req_type = udi_unpack_uint64_t(tmp_req_type);
        if ( (errnum = write_all(response_handle, &tmp_req_type,
                        sizeof(udi_response_type))) != 0 ) break;

        udi_length tmp_length = response->length;
        tmp_length = udi_unpack_uint64_t(tmp_length);
        if ( (errnum = write_all(response_handle, &tmp_length,
                        sizeof(udi_length))) != 0 ) break;

        if ( (errnum = write_all(response_handle, response->value,
                        response->length)) != 0 ) break;
    }while(0);

    if ( errnum != 0 ) {
        udi_printf("failed to send response: %s\n", strerror(errnum));
    }

    return errnum;
}

int write_response(udi_response *response) {
    return write_response_to_fd(response_handle, response);
}

int write_event_to_fd(int fd, udi_event *event) {
    int errnum = 0;
    do {
        udi_event_type tmp_type = event->event_type;
        tmp_type = udi_unpack_uint64_t(tmp_type);
        if ( (errnum = write_all(events_handle, &tmp_type,
                        sizeof(udi_length)) != 0 ) ) break;

        udi_length tmp_length = event->length;
        tmp_length = udi_unpack_uint64_t(tmp_length);
        if ( (errnum = write_all(events_handle, &tmp_length,
                        sizeof(udi_length))) != 0 ) break;

        if ( (errnum = write_all(events_handle, event->data,
                        event->length)) != 0 ) break;
    }while(0);

    if ( errnum != 0 ) {
        udi_printf("failed to send event notification: %s\n",
                strerror(errnum));
    }

    return errnum;
}

int write_event(udi_event *event) {
    return write_event_to_fd(events_handle, event);
}

int continue_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    // TODO
    return 0;
}

int read_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    // TODO
    return 0;
}

int write_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    // TODO
    return 0;
}

int state_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    // TODO
    return 0;
}

int init_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    return 0;
}

static
int wait_and_execute_command(char *errmsg, unsigned int errmsg_size) {
    udi_request *req = NULL;
    int errnum = 0;
    do {
        udi_request *req = read_request();
        if ( req == NULL ) {
            snprintf(errmsg, errmsg_size, "%s", "failed to read command");
            udi_printf("failed to read request\n");
            errnum = -1;
            break;
        }

        errnum = req_handlers[req->request_type](req, errmsg, errmsg_size);
        if ( errnum != 0 ) {
            udi_printf("Failed to handle command %s\n", 
                    request_type_str(req->request_type));
            break;
        }
    }while(0);

    if (req != NULL) free_request(req);

    return errnum;
}

static 
int create_udi_filesystem() {
    int errnum = 0;

    do {
        // get root directory
        if ((UDI_ROOT_DIR = getenv(UDI_ROOT_DIR_ENV)) == NULL) {
            UDI_ROOT_DIR = (char *)udi_malloc(strlen(DEFAULT_UDI_ROOT_DIR));
            strncpy(UDI_ROOT_DIR, DEFAULT_UDI_ROOT_DIR,
                    strlen(DEFAULT_UDI_ROOT_DIR));
        }

        // create the directory for this process
        size_t basedir_length = strlen(UDI_ROOT_DIR) + PID_STR_LEN +
                                DS_LEN;
        basedir_name = (char *)udi_malloc(basedir_length);
        if (basedir_name == NULL) {
            udi_printf("malloc failed: %s\n", strerror(errno));
            errnum = errno;
            break;
        }

        basedir_name[basedir_length-1] = '\0';
        snprintf(basedir_name, basedir_length-1, "%s/%d", UDI_ROOT_DIR,
                 getpid());
        if (mkdir(basedir_name, S_IRWXG | S_IRWXU) == -1) {
            udi_printf("error creating basedir: %s\n", strerror(errno));
            errnum = errno;
            break;
        }

        // create the udi files
        size_t requestfile_length = strlen(UDI_ROOT_DIR) + PID_STR_LEN
                                    + strlen(REQUEST_FILE_NAME) + DS_LEN*2;
        requestfile_name = (char *)udi_malloc(requestfile_length);
        if (requestfile_name == NULL) {
            udi_printf("malloc failed: %s\n", strerror(errno));
            errnum = errno;
            break;
        }

        requestfile_name[requestfile_length-1] = '\0';
        snprintf(requestfile_name, requestfile_length-1, "%s/%d/%s",
                 UDI_ROOT_DIR, getpid(), REQUEST_FILE_NAME);
        if (mkfifo(requestfile_name, S_IRWXG | S_IRWXU) == -1) {
            udi_printf("error creating request file fifo: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }

        size_t responsefile_length = strlen(UDI_ROOT_DIR) + PID_STR_LEN
                                     + strlen(REQUEST_FILE_NAME) + DS_LEN*2;
        responsefile_name = (char *)udi_malloc(responsefile_length);
        if (responsefile_name == NULL) {
            udi_printf("malloc failed: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }

        responsefile_name[responsefile_length-1] = '\0';
        snprintf(responsefile_name, responsefile_length-1, "%s/%d/%s",
                 UDI_ROOT_DIR, getpid(), EVENTS_FILE_NAME);
        if (mkfifo(responsefile_name, S_IRWXG | S_IRWXU) == -1) {
            udi_printf("error creating response file fifo: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }

        size_t eventsfile_length = strlen(UDI_ROOT_DIR) + PID_STR_LEN
                                   + strlen(EVENTS_FILE_NAME) + DS_LEN*2;
        eventsfile_name = (char *)udi_malloc(eventsfile_length);
        if (eventsfile_name == NULL) {
            udi_printf("malloc failed: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }

        eventsfile_name[eventsfile_length-1] = '\0';
        snprintf(eventsfile_name, eventsfile_length-1, "%s/%d/%s",
                 UDI_ROOT_DIR, getpid(), EVENTS_FILE_NAME);
        if (mkfifo(eventsfile_name, S_IRWXG | S_IRWXU) == -1) {
            udi_printf("error creating event file fifo: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }
    }while(0);

    return errnum;
}

static
int locate_wrapper_functions(char *errmsg, unsigned int errmsg_size) {
    int errnum = 0;
    char *errmsg_tmp;

    // reset dlerror()
    dlerror();

    do {
        // locate symbols for wrapper functions
        real_sigaction = (sigaction_type) dlsym(RTLD_NEXT, "sigaction");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg, errmsg_tmp, errmsg_size-1);
            errnum = -1;
            break;
        }

        real_fork = (fork_type) dlsym(RTLD_NEXT, "fork");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg, errmsg_tmp, errmsg_size-1);
            errnum = -1;
            break;
        }

        real_execve = (execve_type) dlsym(RTLD_NEXT, "execve");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg, errmsg_tmp, errmsg_size-1);
            errnum = -1;
            break;
        }
    }while(0);

    return errnum;
}

static
int handshake_with_debugger(int *output_enabled, char *errmsg, 
        unsigned int errmsg_size)
{
    int errnum = 0;

    do {
        if ((request_handle = open(requestfile_name, O_RDONLY))
                == -1 ) {
            udi_printf("error open request file fifo: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }

        udi_request *init_request = read_request();
        if ( init_request == NULL ) {
            snprintf(errmsg, ERRMSG_SIZE-1, "%s", 
                    "failed reading init request");
            udi_printf("failed reading init request");
            errnum = -1;
            break;
        }
        
        if ( init_request->request_type != UDI_REQ_INIT ) {
            udi_printf("invalid init request received, proceeding anyways...\n");
        }

        free_request(init_request);

        if ((response_handle = open(responsefile_name, O_WRONLY))
                == -1 ) {
            udi_printf("error open response file fifo: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }
        *output_enabled = 1;

        if ((events_handle = open(eventsfile_name, O_WRONLY))
                == -1 ) {
            udi_printf("error open response file fifo: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }

        // after sending init response, any debugging operations are valid
        udi_enabled = 1;

        // create the init response
        udi_response init_response;
        init_response.response_type = UDI_RESP_VALID;
        init_response.request_type = UDI_REQ_INIT;
        init_response.length = 0;
        init_response.value = NULL;

        errnum = write_response(&init_response);
        if ( errnum ) {
            udi_printf("failed to write init response\n");
            *output_enabled = 0;
        }
    }while(0);

    return errnum;
}

static
int signals[] = {
    SIGHUP,
    SIGINT,
    SIGQUIT,
    SIGILL,
    SIGTRAP,
    SIGABRT,
    SIGIOT,
    SIGBUS,
    SIGFPE,
    SIGUSR1,
    SIGSEGV,
    SIGUSR2,
    SIGPIPE,
    SIGALRM,
    SIGTERM,
    SIGSTKFLT,
    SIGCHLD,
    SIGCONT,
    SIGTSTP,
    SIGTTIN,
    SIGTTOU,
    SIGURG,
    SIGXCPU,
    SIGXFSZ,
    SIGVTALRM,
    SIGPROF,
    SIGWINCH,
    SIGIO,
    SIGPWR,
    SIGSYS
};

static
int decode_trap(const siginfo_t *siginfo, const ucontext_t *context, 
        char *errmsg, unsigned int errmsg_size) 
{
    udi_address trap_addr = (udi_address) siginfo->si_addr;

    udi_event trap_event;
    trap_event.event_type = UDI_EVENT_BREAKPOINT;
    trap_event.length = sizeof(udi_address);
    trap_event.data = &trap_addr;

    return write_event(&trap_event); 
}

// Not static because need to pass around pointer to it
void signal_entry_point(int signal, siginfo_t *siginfo, void *v_context) {
    int failure = 0;
    char errmsg[ERRMSG_SIZE];
    errmsg[ERRMSG_SIZE-1] = '\0';

    ucontext_t *context = (ucontext_t *)v_context;

    udi_in_sig_handler++;

    // create event
    // Note: each decoder function will call write_event to avoid unnecessary
    // heap allocations
    do {
        udi_event event;
        switch(signal) {
            case SIGTRAP:
                failure = decode_trap(siginfo, context, errmsg, 
                        ERRMSG_SIZE);
                break;
            default:
                event.event_type = UDI_EVENT_UNKNOWN;
                event.length = 0;
                event.data = NULL;

                failure = write_event(&event);
                break;
        }

        if ( failure ) {
            if ( failure > 0 ) {
                strncpy(errmsg, strerror(failure), ERRMSG_SIZE-1);
            }
            break;
        }
    
        // wait for command
        if ( (failure = wait_and_execute_command(errmsg, ERRMSG_SIZE)) != 0 ) {
            if ( failure > 0 ) {
                strncpy(errmsg, strerror(failure), ERRMSG_SIZE-1);
            }
            break;
        }
    }while(0);

    if ( failure ) {
        udi_response resp;
        resp.response_type = UDI_RESP_ERROR;
        resp.request_type = UDI_REQ_INVALID;
        resp.length = strlen(errmsg);
        resp.value = errmsg;
        
        // explicitly ignore errors
        write_response(&resp);
    }

    udi_in_sig_handler--;
}

static
int setup_signal_handlers() {
    int errnum = 0;

    int length = sizeof(signals) / sizeof(int);

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));

    action.sa_sigaction = signal_entry_point;
    sigfillset(&(action.sa_mask));
    action.sa_flags = SA_SIGINFO;

    int i;
    for(i = 0; i < length; ++i) {
        if ( real_sigaction(i, &action, NULL) != 0 ) {
            errnum = errno;
            break;
        }
    }

    return errnum;
}

void init_udi_rt() UDI_CONSTRUCTOR;

void init_udi_rt() {
    char errmsg[ERRMSG_SIZE];
    char *errmsg_tmp;
    int errnum = 0, output_enabled = 0;

    // initialize error message
    errmsg[ERRMSG_SIZE-1] = '\0';

    // turn on debugging, if necessary
    if (getenv(UDI_DEBUG_ENV) != NULL) {
        udi_debug_on = 1;
    }

    do {
        if ( (errnum = create_udi_filesystem()) != 0 ) {
            udi_printf("failed to create udi filesystem\n");
            break;
        }

        if ( (errnum = locate_wrapper_functions(errmsg, ERRMSG_SIZE)) 
                != 0 ) {
            udi_printf("failed to locate wrapper functions\n");
            break;
        }

        if ( (errnum = setup_signal_handlers()) != 0 ) {
            udi_printf("failed to setup signal handlers\n");
            break;
        }

        if ( (errnum = handshake_with_debugger(&output_enabled,
                        errmsg, ERRMSG_SIZE)) != 0 ) {
            udi_printf("failed to complete handshake with debugger\n");
            break;
        }

        if ( (errnum = wait_and_execute_command(errmsg, ERRMSG_SIZE)) != 0 ) {
            udi_printf("failed to wait for initial command\n");
            break;
        }
    } while(0);

    if (errnum > 0) {
        strerror_r(errnum, errmsg, ERRMSG_SIZE-1);
    }

    if (errnum != 0) {
        if(!output_enabled) {
            fprintf(stderr, "Failed to initialize udi rt: %s\n", errmsg);
        } else {
            // explicitly don't worry about return
            udi_response resp;
            resp.response_type = UDI_RESP_ERROR;
            resp.request_type = UDI_REQ_INIT;
            resp.length = strlen(errmsg);
            resp.value = errmsg;
            write_response(&resp);
        }

        // not enabled, due to failure
        udi_enabled = 0;
    }
}
