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

// This needs to be included first because it sets feature macros
#include "udirt-platform.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ucontext.h>

#include "udirt.h"
#include "udi.h"
#include "udi-common.h"
#include "udi-common-posix.h"


////////////////////////////////////
// Definitions
////////////////////////////////////

// macros
#define UDI_CONSTRUCTOR __attribute__((constructor))
#define CASE_TO_STR(x) case x: return #x

// testing
extern int testing_udirt(void) __attribute__((weak));

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

// write/read permission fault handling
static int performing_mem_access = 0;
static void *mem_access_addr = NULL;
static size_t mem_access_size = 0;

static int abort_mem_access = 0;
static int failed_si_code = 0;

// write failure handling
static int pipe_write_failure = 0;

// Signal handling
static
int signals[] = {
    SIGHUP,
    SIGINT,
    SIGQUIT,
    SIGILL,
    SIGTRAP,
    SIGABRT,
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

// This is the number of elements in the signals array
#define NUM_SIGNALS 29

static struct sigaction app_actions[NUM_SIGNALS];

// Used to map signals to their application action
static int signal_map[MAX_SIGNAL_NUM];

static struct sigaction default_lib_action;

extern int pthread_sigmask(int how, const sigset_t *new_set, sigset_t *old_set) __attribute__((weak));

static int pass_signal = 0;

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

/** Request processed successfully */
static const int REQ_SUCCESS = 0;

/** Failure to process request due to environment/OS error, unrecoverable */
static const int REQ_ERROR = -1;

/** Failure to process request due to invalid arguments */
static const int REQ_FAILURE = -2;

////////////////////////////////////
// Functions
////////////////////////////////////

static
void disable_debugging() {
    udi_enabled = 0;

    // Replace the library signal handler with the application signal handlers
    int i;
    for(i = 0; i < NUM_SIGNALS; ++i) {
        if ( signals[i] == SIGPIPE && pipe_write_failure ) continue;

        if ( real_sigaction(signals[i], &app_actions[i], NULL) != 0 ) {
            udi_printf("Failed to reset signal handler for %d: %s\n",
                    signals[i], strerror(errno));
        }
    }

    udi_printf("%s\n", "Disabled debugging");
}

static
int setsigmask(int how, const sigset_t *new_set, sigset_t *old_set) {
    // Only use pthread_sigmask when it is available
    if ( pthread_sigmask ) {
        // TODO block signals for all threads
        return pthread_sigmask(how, new_set, old_set);
    }

    return sigprocmask(how, new_set, old_set);
}

static
void handle_pipe_write_failure() {
    pipe_write_failure = 1;

    sigset_t set;

    if ( sigpending(&set) != 0 ) {
        udi_printf("Failed to get pending signals: %s\n", strerror(errno));
        return;
    }

    if ( sigismember(&set, SIGPIPE) != 1 ) {
        udi_printf("%s\n", "SIGPIPE is not pending, cannot handle write failure");
        return;
    }

    sigset_t cur_set;
    if ( setsigmask(SIG_BLOCK, NULL, &cur_set) != 0 ) {
        udi_printf("Failed to get current signals: %s\n", strerror(errno));
        return;
    }
    sigdelset(&set, SIGPIPE);

    sigsuspend(&set);
    if ( errno != EINTR ) {
        udi_printf("Failed to wait for signal to be delivered: %s\n",
                strerror(errno));
        return;
    }

    pipe_write_failure = 0;
}

static
void app_signal_handler(int signal, siginfo_t *siginfo, void *v_context) {
    int signal_index = signal_map[(signal % MAX_SIGNAL_NUM)];

    if ( (void *)app_actions[signal_index].sa_sigaction == (void *)SIG_IGN ) {
        udi_printf("Signal %d ignored, not passing to application\n", signal);
        return;
    }

    if ( (void *)app_actions[signal_index].sa_sigaction == (void *)SIG_DFL ) {
        // TODO
        return;
    }

    sigset_t cur_set;

    if ( setsigmask(SIG_SETMASK, &app_actions[signal_index].sa_mask, 
            &cur_set) != 0 )
    {
        udi_printf("Failed to adjust blocked signals for application handler: %s\n",
                strerror(errno));
    }

    app_actions[signal_index].sa_sigaction(signal, siginfo, v_context);

    if ( setsigmask(SIG_SETMASK, &cur_set, NULL) != 0 ) {
        udi_printf("Failed to reset blocked signals after application handler: %s\n",
                strerror(errno));
    }
}

// wrapper function implementations

int sigaction(int signum, const struct sigaction *act,
        struct sigaction *oldact)
{
    // Block signals while doing this to avoid a race where a signal is delivered
    // while validating the arguments
    sigset_t full_set, orig_set;
    sigfillset(&full_set);

    int block_result = setsigmask(SIG_SETMASK, &full_set, &orig_set);
    if ( block_result != 0 ) return EINVAL;

    // Validate the arguments
    int result = real_sigaction(signum, act, oldact);
    if ( result != 0 ) return result;

    // Reset action back to library default
    result = real_sigaction(signum, &default_lib_action, NULL);
    if ( result != 0 ) return result;

    // Store new application action for future use
    int signal_index = signal_map[(signum % MAX_SIGNAL_NUM)];
    if ( oldact != NULL ) {
        *oldact = app_actions[signal_index];
    }

    if ( act != NULL ) {
        app_actions[signal_index] = *act;
    }

    // Unblock signals
    block_result = setsigmask(SIG_SETMASK, &orig_set, NULL);
    if ( block_result != 0 ) return EINVAL;

    return 0;
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

void *udi_malloc(size_t length) {
    return malloc(length);
}

// request-response handling

void free_request(udi_request *request) {
    if ( request->packed_data != NULL ) udi_free(request->packed_data);
    udi_free(request);
}

udi_request *read_request_from_fd(int fd) {
    // easier to test if broken out into a parameterized function

    int errnum = 0;
    udi_request *request = (udi_request *)udi_malloc(sizeof(udi_request));

    if (request == NULL) {
        udi_printf("malloc failed: %s\n", strerror(errno));
        return NULL;
    }

    request->packed_data = NULL;
    do {
        // read the request type and length
        if ( (errnum = read_all(fd, &(request->request_type),
                        sizeof(udi_request_type))) != 0 ) break;
        request->request_type = udi_request_type_ntoh(request->request_type);

        if ( (errnum = read_all(fd, &(request->length),
                    sizeof(udi_length))) != 0) break;
        request->length = udi_length_ntoh(request->length);

        // read the payload
        if ( request->length > 0 ) {
            request->packed_data = udi_malloc(request->length);
            if (request->packed_data == NULL ) {
                errnum = errno;
                break;
            }

            if ( (errnum = read_all(fd, request->packed_data,
                        request->length)) != 0) break;
        }
    }while(0);

    if (errnum != 0) {
        if ( errnum > 0 ) {
            udi_printf("read_all failed: %s\n", strerror(errnum));
        }

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
        tmp_type = udi_response_type_hton(tmp_type);
        if ( (errnum = write_all(response_handle, &tmp_type,
                       sizeof(udi_response_type))) != 0 ) break;

        udi_request_type tmp_req_type = response->request_type;
        tmp_req_type = udi_request_type_hton(tmp_req_type);
        if ( (errnum = write_all(response_handle, &tmp_req_type,
                        sizeof(udi_response_type))) != 0 ) break;

        udi_length tmp_length = response->length;
        tmp_length = udi_length_hton(tmp_length);
        if ( (errnum = write_all(response_handle, &tmp_length,
                        sizeof(udi_length))) != 0 ) break;

        if ( (errnum = write_all(response_handle, response->packed_data,
                        response->length)) != 0 ) break;
    }while(0);

    if ( errnum != 0 ) {
        udi_printf("failed to send response: %s\n", strerror(errnum));
    }

    if ( errnum == EPIPE ) {
        handle_pipe_write_failure();
    }

    return errnum;
}

int write_response(udi_response *response) {
    return write_response_to_fd(response_handle, response);
}

int write_response_to_request(udi_response *response) {
    int write_result = write_response(response);

    if ( write_result < 0 ) return REQ_ERROR;
    if ( write_result > 0 ) return write_result;
    return REQ_SUCCESS;
}

int write_event_to_fd(int fd, udi_event_internal *event) {
    int errnum = 0;

    do {
        udi_event_type tmp_type = event->event_type;
        tmp_type = udi_event_type_hton(tmp_type);
        if ( (errnum = write_all(events_handle, &tmp_type,
                        sizeof(udi_event_type)) != 0 ) ) break;

        udi_length tmp_length = event->length;
        tmp_length = udi_length_hton(tmp_length);
        if ( (errnum = write_all(events_handle, &tmp_length,
                        sizeof(udi_length))) != 0 ) break;

        if ( (errnum = write_all(events_handle, event->packed_data,
                        event->length)) != 0 ) break;
    }while(0);

    if ( errnum != 0 ) {
        udi_printf("failed to send event notification: %s\n",
                strerror(errnum));
    }

    if ( errnum == EPIPE ) {
        handle_pipe_write_failure();
    }

    return errnum;
}

int write_event(udi_event_internal *event) {
    return write_event_to_fd(events_handle, event);
}

static
int abortable_memcpy(void *dest, const void *src, size_t n) {
    /* slow as molasses, but gets the job done */
    unsigned char *uc_dest = (unsigned char *)dest;
    const unsigned char *uc_src = (const unsigned char *)src;

    size_t i = 0;
    for (i = 0; i < n && !abort_mem_access; ++i) {
        uc_dest[i] = uc_src[i];
    }

    if ( abort_mem_access ) {
        abort_mem_access = 0;
        return -1;
    }

    return 0;
}

static
int failed_mem_access_response(udi_request_type request_type, char *errmsg,
        unsigned int errmsg_size)
{
    udi_response resp;
    resp.response_type = UDI_RESP_ERROR;
    resp.request_type = request_type;

    char *errstr;
    switch(failed_si_code) {
        case SEGV_MAPERR:
            errstr = "address not mapped in process";
            break;
        default:
            errstr = "unknown memory error";
            break;
    }

    resp.length = strlen(errstr);
    resp.packed_data = udi_pack_data(resp.length, UDI_DATATYPE_BYTESTREAM,
        resp.length, errstr);

    int errnum = 0;
    do {
        if ( resp.packed_data == NULL ) {
            snprintf(errmsg, errmsg_size, "%s", "failed to pack response data");
            udi_printf("%s", "failed to pack response data for read request");
            errnum = -1;
            break;
        }

        errnum = write_response(&resp);
    }while(0);

    if ( resp.packed_data != NULL ) udi_free(resp.packed_data);

    return errnum;
}

///////////////////////
// Handler functions //
///////////////////////

int continue_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    // for now, don't do anything special, just send the response
    // when threads introduced, this will require more work

    uint32_t sig_val;
    if ( udi_unpack_data(req->packed_data, req->length,
                UDI_DATATYPE_INT32, &sig_val) )
    {
        snprintf(errmsg, errmsg_size, "%s", "failed to parse continue request");
        udi_printf("%s\n", "failed to parse continue request");
        return REQ_FAILURE;
    }

    if ( sig_val < 0 ) {
        snprintf(errmsg, errmsg_size, "invalid signal specified: %d", sig_val);
        udi_printf("invalid signal specified %d\n", sig_val);
        return REQ_FAILURE;
    }
    
    udi_response resp;
    resp.response_type = UDI_RESP_VALID;
    resp.request_type = UDI_REQ_CONTINUE;
    resp.length = 0;
    resp.packed_data = NULL;

    int result = write_response_to_request(&resp);

    if ( result == REQ_SUCCESS ) {
        pass_signal = sig_val;
        kill(getpid(), sig_val);
    }

    return result;
}

int read_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    udi_address addr;
    udi_length num_bytes;

    if ( udi_unpack_data(req->packed_data, req->length, 
                UDI_DATATYPE_ADDRESS, &addr, UDI_DATATYPE_LENGTH,
                &num_bytes) ) 
    {
        snprintf(errmsg, errmsg_size, "%s", "failed to parse read request");
        udi_printf("%s\n", "failed to unpack data for read request");
        return REQ_FAILURE;
    }

    void *memory_read = udi_malloc(num_bytes);
    if ( memory_read == NULL ) {
        return errno;
    }

    // Perform the read operation
    performing_mem_access = 1;
    mem_access_addr = (void *)(unsigned long)addr;
    mem_access_size = num_bytes;
    if ( abortable_memcpy(memory_read, (void *)((unsigned long)addr), num_bytes) 
            != 0 )
    {
        if (memory_read != NULL) udi_free(memory_read);
        int mem_result = failed_mem_access_response(UDI_REQ_READ_MEM, errmsg, errmsg_size);

        if ( mem_result < 0 ) {
            return REQ_ERROR;
        }else if ( mem_result > 0 ) {
            return mem_result;
        }
    }
    performing_mem_access = 0;

    // Create the response
    udi_response resp;
    resp.response_type = UDI_RESP_VALID;
    resp.request_type = UDI_REQ_READ_MEM;
    resp.length = num_bytes;
    resp.packed_data = udi_pack_data(resp.length, UDI_DATATYPE_BYTESTREAM,
            resp.length, memory_read);

    int result = 0;
    do {
        if ( resp.packed_data == NULL ) {
            snprintf(errmsg, errmsg_size, "%s", "failed to pack response data");
            udi_printf("%s", "failed to pack response data for read request");
            result = REQ_ERROR;
            break;
        }

        result = write_response_to_request(&resp);
    }while(0);

    if ( memory_read != NULL ) udi_free(memory_read);
    if ( resp.packed_data != NULL ) udi_free(resp.packed_data);

    return result;
}

int write_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    udi_address addr;
    udi_length num_bytes;
    void *bytes_to_write;

    if ( udi_unpack_data(req->packed_data, req->length,
                UDI_DATATYPE_ADDRESS, &addr, UDI_DATATYPE_BYTESTREAM, &num_bytes,
                &bytes_to_write) )
    {
        snprintf(errmsg, errmsg_size, "%s", "failed to parse write request");
        udi_printf("%s\n", "failed to unpack data for write request");
        return REQ_FAILURE;
    }
    
    // Perform the write operation
    performing_mem_access = 1;
    mem_access_addr = (void *)(unsigned long)addr;
    mem_access_size = num_bytes;
    if ( abortable_memcpy((void *)((unsigned long)addr), bytes_to_write, num_bytes)
            != 0 )
    {
        udi_free(bytes_to_write);
        int mem_result = failed_mem_access_response(UDI_REQ_WRITE_MEM, errmsg, errmsg_size);

        if ( mem_result < 0 ) {
            return REQ_ERROR;
        }else if ( mem_result > 0 ) {
            return mem_result;
        }
    }
    performing_mem_access = 0;

    // Create the response
    udi_response resp;
    resp.response_type = UDI_RESP_VALID;
    resp.request_type = UDI_REQ_WRITE_MEM;
    resp.length = 0;
    resp.packed_data = NULL;

    int write_result = write_response_to_request(&resp);

    udi_free(bytes_to_write);

    return write_result;
}

int state_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    // TODO
    return 0;
}

int init_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    return 0;
}

///////////////////////////
// End handler functions //
///////////////////////////

static
int wait_and_execute_command(char *errmsg, unsigned int errmsg_size) {
    udi_request *req = NULL;
    int result = 0;

    while(1) {
        udi_request *req = read_request();
        if ( req == NULL ) {
            snprintf(errmsg, errmsg_size, "%s", "failed to read command");
            udi_printf("%s\n", "failed to read request");
            result = REQ_ERROR;
            break;
        }

        result = req_handlers[req->request_type](req, errmsg, errmsg_size);
        if ( result != REQ_SUCCESS ) {
            udi_printf("Failed to handle command %s\n", 
                    request_type_str(req->request_type));
            break;
        }

        if ( req->request_type == UDI_REQ_CONTINUE ) break;
    }

    if (req != NULL) free_request(req);

    return result;
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
                                     + strlen(RESPONSE_FILE_NAME) + DS_LEN*2;
        responsefile_name = (char *)udi_malloc(responsefile_length);
        if (responsefile_name == NULL) {
            udi_printf("malloc failed: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }

        responsefile_name[responsefile_length-1] = '\0';
        snprintf(responsefile_name, responsefile_length-1, "%s/%d/%s",
                 UDI_ROOT_DIR, getpid(), RESPONSE_FILE_NAME);
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
            udi_printf("error opening request file fifo: %s\n",
                       strerror(errno));
            errnum = errno;
            break;
        }

        udi_request *init_request = read_request();
        if ( init_request == NULL ) {
            snprintf(errmsg, errmsg_size-1, "%s", 
                    "failed reading init request");
            udi_printf("%s\n", "failed reading init request");
            errnum = -1;
            break;
        }
        
        if ( init_request->request_type != UDI_REQ_INIT ) {
            udi_printf("%s\n",
                    "invalid init request received, proceeding anyways...");
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
        init_response.packed_data = NULL;

        errnum = write_response(&init_response);
        if ( errnum ) {
            udi_printf("%s\n", "failed to write init response");
            *output_enabled = 0;
        }
    }while(0);

    return errnum;
}

static
int decode_segv(const siginfo_t *siginfo, const ucontext_t *context,
        char *errmsg, unsigned int errmsg_size)
{
    int errnum = 0;
    if ( performing_mem_access ) {
        // if the error was due to permissions, change permissions temporarily
        // to allow mem access to complete
        if (siginfo->si_code == SEGV_ACCERR) {
            unsigned long mem_access_addr_ul = (unsigned long)mem_access_addr;

            // page align
            mem_access_addr_ul = mem_access_addr_ul & ~(sysconf(_SC_PAGESIZE)-1);

            // Note: this could cause unwanted side effects in the debuggee
            // because it could mask errors that the debuggee should encounter.
            // However, in order to handle this case, the protection for a page
            // must be queried from the OS, and currently there is not a
            // portable way to do this. Thus, the tradeoff was made in favor of
            // portability.
            if ( mprotect((void *)mem_access_addr_ul, mem_access_size,
                        PROT_READ | PROT_WRITE | PROT_EXEC) != 0 ) 
            {
                errnum = errno;
                udi_printf("failed to modify permissions for memory access: %s\n",
                        strerror(errnum));
            }
        }else{
            errnum = -1;
        }

        if (errnum != 0) {
            abort_mem_access = 1;
            failed_si_code = siginfo->si_code;
        }
    }else{
        // TODO create event and send to debugger
    }

    if ( errnum > 0 ) {
        strerror_r(errnum, errmsg, errmsg_size);
    }

    return errnum;
}

// Not static because need to pass around pointer to it
void signal_entry_point(int signal, siginfo_t *siginfo, void *v_context) {
    int failure = 0, request_error = 0;
    char errmsg[ERRMSG_SIZE];
    errmsg[ERRMSG_SIZE-1] = '\0';

    ucontext_t *context = (ucontext_t *)v_context;

    if ( pipe_write_failure && signal == SIGPIPE ) {
        udi_printf("%s\n", "Ignoring SIGPIPE due to previous library write failure");
        pipe_write_failure = 0;

        real_sigaction(signal, &app_actions[signal_map[signal % MAX_SIGNAL_NUM]], NULL);

        return;
    }

    if ( pass_signal != 0 ) {
        app_signal_handler(signal, siginfo, v_context);
        pass_signal = 0;
        return;
    }

    if (!udi_enabled) {
        udi_printf("UDI disabled, not handling signal %d at addr 0x%08lx\n", signal,
                (unsigned long)siginfo->si_addr);
        return;
    }

    udi_in_sig_handler++;

    // create event
    // Note: each decoder function will call write_event to avoid unnecessary
    // heap allocations
    do {
        udi_event_internal event;
        switch(signal) {
            case SIGSEGV:
                failure = decode_segv(siginfo, context, errmsg,
                        ERRMSG_SIZE);
                break;
            default:
                event.event_type = UDI_EVENT_UNKNOWN;
                event.length = 0;
                event.packed_data = NULL;

                failure = write_event(&event);
                break;
        }

        if ( failure != 0 ) {
            if ( failure > 0 ) {
                strncpy(errmsg, strerror(failure), ERRMSG_SIZE-1);
            }

            event.event_type = UDI_EVENT_ERROR;
            event.length = strlen(errmsg) + 1;
            event.packed_data = udi_pack_data(event.length,
                    UDI_DATATYPE_BYTESTREAM, event.length, errmsg);

            // Explicitly ignore any errors -- no way to report them
            if ( event.packed_data != NULL ) {
                write_event(&event);
                udi_free(event.packed_data);
            }

            failure = 0;
            break;
        }
    
        // wait for command
        if ( !performing_mem_access ) {
            int req_result = wait_and_execute_command(errmsg, ERRMSG_SIZE);
            if ( req_result != REQ_SUCCESS ) {
                failure = 1;

                if ( req_result == REQ_FAILURE ) {
                    request_error = 0;
                }else if ( req_result == REQ_ERROR ) {
                    request_error = 1;
                }else if ( req_result > REQ_SUCCESS ) {
                    request_error = 1;
                    strncpy(errmsg, strerror(req_result), ERRMSG_SIZE-1);
                }
                break;
            }
        }
    }while(0);

    if ( failure ) {
        // A failed request most likely means the debugger is no longer around, so
        // don't try to send a request
        if ( !request_error ) {
            // Shared error response code
            udi_response resp;
            resp.response_type = UDI_RESP_ERROR;
            resp.request_type = UDI_REQ_INVALID;
            resp.length = strlen(errmsg) + 1;
            resp.packed_data = udi_pack_data(resp.length, 
                    UDI_DATATYPE_BYTESTREAM, resp.length, errmsg);

            if ( resp.packed_data != NULL ) {
                // explicitly ignore errors
                write_response(&resp);
                udi_free(resp.packed_data);
            }
        }else{
            udi_printf("%s\n", "disabling UDI due to request failure");
            disable_debugging();
        }
    }

    udi_in_sig_handler--;
}

static
int setup_signal_handlers() {
    int errnum = 0;

    // Sanity check
    if ( (sizeof(signals) / sizeof(int)) != NUM_SIGNALS ) {
        udi_printf("%s\n", "ASSERT FAIL: signals array length != NUM_SIGNALS");
        return -1;
    }

    int i;
    for(i = 0; i < NUM_SIGNALS; ++i) {
        if ( real_sigaction(signals[i], &default_lib_action, &app_actions[i]) != 0 ) {
            errnum = errno;
            break;
        }
    }

    return errnum;
}

static
void global_variable_initialization() {
    // turn on debugging, if necessary
    if (getenv(UDI_DEBUG_ENV) != NULL) {
        udi_debug_on = 1;
        udi_printf("%s\n", "UDI rt debug logging enabled");
    }

    // set allocator used for packing data
    udi_set_malloc(udi_malloc);

    // Define the default sigaction for the library
    memset(&default_lib_action, 0, sizeof(struct sigaction));
    default_lib_action.sa_sigaction = signal_entry_point;
    sigfillset(&(default_lib_action.sa_mask));
    default_lib_action.sa_flags = SA_SIGINFO | SA_NODEFER;

    // initialize application sigactions and signal map
    int i;
    for (i = 0; i < NUM_SIGNALS; ++i) {
        memset(&app_actions[i], 0, sizeof(struct sigaction));
        app_actions[i].sa_handler = SIG_DFL;

        signal_map[(signals[i] % MAX_SIGNAL_NUM)] = i;
    }
}

void init_udi_rt() UDI_CONSTRUCTOR;

void init_udi_rt() {
    char errmsg[ERRMSG_SIZE];
    int errnum = 0, output_enabled = 0;

    // initialize error message
    errmsg[ERRMSG_SIZE-1] = '\0';

    if ( testing_udirt != NULL ) return;

    global_variable_initialization();
   
    sigset_t original_set;
    do {
        sigset_t block_set;
        sigfillset(&block_set);

        // Block any signals during initialization
        if ( setsigmask(SIG_SETMASK, &block_set, &original_set) 
                == -1 ) 
        {
            errnum = errno;
            udi_printf("failed to block all signals: %s\n", strerror(errnum));
            break;
        }

        if ( (errnum = locate_wrapper_functions(errmsg, ERRMSG_SIZE)) 
                != 0 ) 
        {
            udi_printf("%s\n", "failed to locate wrapper functions");
            break;
        }

        if ( (errnum = setup_signal_handlers()) != 0 ) {
            udi_printf("%s\n", "failed to setup signal handlers");
            break;
        }

        if ( (errnum = create_udi_filesystem()) != 0 ) {
            udi_printf("%s\n", "failed to create udi filesystem");
            break;
        }
        

        if ( (errnum = handshake_with_debugger(&output_enabled,
                        errmsg, ERRMSG_SIZE)) != 0 ) {
            udi_printf("%s\n", "failed to complete handshake with debugger");
            break;
        }

        if ( (errnum = wait_and_execute_command(errmsg, ERRMSG_SIZE)) != 0 ) {
            udi_printf("%s\n", "failed to wait for initial command");
            break;
        }
    } while(0);

    if (errnum > 0) {
        strerror_r(errnum, errmsg, ERRMSG_SIZE-1);
    }

    if (errnum != 0) {
        udi_enabled = 0;

        if(!output_enabled) {
            fprintf(stderr, "Failed to initialize udi rt: %s\n", errmsg);
        } else {
            // explicitly don't worry about return
            udi_response resp;
            resp.response_type = UDI_RESP_ERROR;
            resp.request_type = UDI_REQ_INIT;
            resp.length = strlen(errmsg) + 1;
            resp.packed_data = udi_pack_data(resp.length, 
                    UDI_DATATYPE_BYTESTREAM, resp.length, errmsg);

            if ( resp.packed_data != NULL ) {
                write_response(&resp);
                udi_free(resp.packed_data);
            }
        }

        disable_debugging();

        udi_printf("%s\n", "Initialization failed");
    }else{
        udi_printf("%s\n", "Initialization completed");
    }

    // Explicitly ignore errors
    setsigmask(SIG_SETMASK, &original_set, NULL);
}
