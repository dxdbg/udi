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

// write/read permission fault handling
static int performing_mem_access = 0;
static void *mem_access_addr = NULL;
static size_t mem_access_size = 0;

static int abort_mem_access = 0;
static int failed_si_code = 0;

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

    return errnum;
}

int write_response(udi_response *response) {
    return write_response_to_fd(response_handle, response);
}

int write_event_to_fd(int fd, udi_event *event) {
    int errnum = 0;
    do {
        udi_event_type tmp_type = event->event_type;
        tmp_type = udi_event_type_hton(tmp_type);
        if ( (errnum = write_all(events_handle, &tmp_type,
                        sizeof(udi_length)) != 0 ) ) break;

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

    return errnum;
}

int write_event(udi_event *event) {
    return write_event_to_fd(events_handle, event);
}

int continue_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    // for now, don't do anything special, just send the response
    
    // when threads introduced, this will require more work
    udi_response resp;

    resp.response_type = UDI_RESP_VALID;
    resp.request_type = UDI_REQ_CONTINUE;
    resp.length = 0;
    resp.packed_data = NULL;

    return write_response(&resp);
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

int read_handler(udi_request *req, char *errmsg, unsigned int errmsg_size) {
    udi_address addr;
    udi_length num_bytes;

    if ( udi_unpack_data(req->packed_data, req->length, 
                UDI_DATATYPE_ADDRESS, &addr, UDI_DATATYPE_LENGTH,
                &num_bytes) ) 
    {
        snprintf(errmsg, errmsg_size, "%s", "failed to parse read request");
        udi_printf("%s", "failed to unpack data for read request");
        return -1;
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
        return failed_mem_access_response(UDI_REQ_READ_MEM, errmsg, errmsg_size);
    }
    performing_mem_access = 0;

    // Create the response
    udi_response resp;
    resp.response_type = UDI_RESP_VALID;
    resp.request_type = UDI_REQ_READ_MEM;
    resp.length = num_bytes;
    resp.packed_data = udi_pack_data(resp.length, UDI_DATATYPE_BYTESTREAM,
            resp.length, memory_read);

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

    if ( memory_read != NULL ) udi_free(memory_read);
    if ( resp.packed_data != NULL ) udi_free(resp.packed_data);

    return errnum;
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
        return -1;
    }
    
    // Perform the write operation
    performing_mem_access = 1;
    mem_access_addr = (void *)(unsigned long)addr;
    mem_access_size = num_bytes;
    if ( abortable_memcpy((void *)((unsigned long)addr), bytes_to_write, num_bytes)
            != 0 )
    {
        udi_free(bytes_to_write);
        return failed_mem_access_response(UDI_REQ_WRITE_MEM, errmsg, errmsg_size);
    }
    performing_mem_access = 0;

    // Create the response
    udi_response resp;
    resp.response_type = UDI_RESP_VALID;
    resp.request_type = UDI_REQ_WRITE_MEM;
    resp.length = 0;
    resp.packed_data = NULL;

    int errnum = write_response(&resp);

    udi_free(bytes_to_write);

    return errnum;
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

    while(1) {
        udi_request *req = read_request();
        if ( req == NULL ) {
            snprintf(errmsg, errmsg_size, "%s", "failed to read command");
            udi_printf("%s\n", "failed to read request");
            errnum = -1;
            break;
        }

        errnum = req_handlers[req->request_type](req, errmsg, errmsg_size);
        if ( errnum != 0 ) {
            udi_printf("Failed to handle command %s\n", 
                    request_type_str(req->request_type));
            break;
        }

        if ( req->request_type == UDI_REQ_CONTINUE ) break;
    }

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
            udi_printf("error open request file fifo: %s\n",
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
    udi_address trap_addr = (udi_address)(unsigned long)siginfo->si_addr;

    udi_event trap_event;
    trap_event.event_type = UDI_EVENT_BREAKPOINT;
    trap_event.length = sizeof(udi_address);
    trap_event.packed_data = udi_pack_data(trap_event.length,
            UDI_DATATYPE_INT64, trap_addr);

    if ( trap_event.packed_data == NULL ) {
        return -1;
    }

    int result = write_event(&trap_event);

    udi_free(trap_event.packed_data);

    return result;

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

        if ( failure ) {
            if ( failure > 0 ) {
                strncpy(errmsg, strerror(failure), ERRMSG_SIZE-1);
            }
            break;
        }
    
        // wait for command
        if ( !performing_mem_access ) {
            if ( (failure = wait_and_execute_command(errmsg, ERRMSG_SIZE)) != 0 ) {
                if ( failure > 0 ) {
                    strncpy(errmsg, strerror(failure), ERRMSG_SIZE-1);
                }
                break;
            }
        }
    }while(0);

    if ( failure ) {
        // Shared error response code
        udi_response resp;
        resp.response_type = UDI_RESP_ERROR;
        resp.request_type = UDI_REQ_INVALID;
        resp.length = strlen(errmsg) + 1;
        resp.packed_data = udi_pack_data(resp.length, 
                UDI_DATATYPE_BYTESTREAM, errmsg);

        if ( resp.packed_data != NULL ) {
            // explicitly ignore errors
            write_response(&resp);
            udi_free(resp.packed_data);
        }
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

    // 0 is not a valid signal to register a signal handler for
    int i;
    for(i = 1; i < length; ++i) {
        if ( i == SIGKILL || i == SIGSTOP ) continue;
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
    int errnum = 0, output_enabled = 0;

    // initialize error message
    errmsg[ERRMSG_SIZE-1] = '\0';

    // turn on debugging, if necessary
    if (getenv(UDI_DEBUG_ENV) != NULL) {
        udi_debug_on = 1;
    }

    // set allocator used for packing data
    udi_set_malloc(udi_malloc);

    do {
        if ( (errnum = locate_wrapper_functions(errmsg, ERRMSG_SIZE)) 
                != 0 ) {
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
        if(!output_enabled) {
            fprintf(stderr, "Failed to initialize udi rt: %s\n", errmsg);
        } else {
            // explicitly don't worry about return
            udi_response resp;
            resp.response_type = UDI_RESP_ERROR;
            resp.request_type = UDI_REQ_INIT;
            resp.length = strlen(errmsg) + 1;
            resp.packed_data = udi_pack_data(resp.length, 
                    UDI_DATATYPE_BYTESTREAM, errmsg);

            if ( resp.packed_data != NULL ) {
                write_response(&resp);
                udi_free(resp.packed_data);
            }
        }

        // not enabled, due to failure
        udi_enabled = 0;
    }
}
