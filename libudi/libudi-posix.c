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

#include "udi.h"
#include "libudi.h"
#include "udi-common.h"
#include "libudi-private.h"
#include "udi-common-posix.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>

const int INVALID_UDI_PID = -1;

int create_root_udi_filesystem() {
    if (mkdir(udi_root_dir, S_IRWXG | S_IRWXU) == -1) {
        if ( errno == EEXIST ) {
            // Make sure it is a directory
            struct stat dirstat;
            if ( stat(udi_root_dir, &dirstat) != 0 ) {
                udi_printf("failed to stat file %s: %s\n",
                        udi_root_dir, strerror(errno));
                return -1;
            }

            if (!S_ISDIR(dirstat.st_mode)) {
                udi_printf("UDI root dir %s exists and is not a directory\n",
                        udi_root_dir);
                return -1;
            }
            // It exists and it is a directory
        }else{
            udi_printf("error creating udi root dir: %s\n",
                strerror(errno));
            return -1;
        }    
    }

    return 0;
}

udi_pid fork_process(const char *executable, char * const argv[],
        char * const envp[])
{
    // First process initialization
    if (processes_created == 0) {
        // Ignore zombies
        struct sigaction sigchild_action;
        memset(&sigchild_action, 0, sizeof(struct sigaction));
        sigchild_action.sa_handler = SIG_IGN;
        sigchild_action.sa_flags = SA_NOCLDWAIT;
        if ( sigaction(SIGCHLD, &sigchild_action, NULL) != 0 ) {
            udi_printf("failed to disable zombie creation: %s\n",
                    strerror(errno));
            return -1;
        }
    }

    // Use the following procedure to get feedback on whether the exec
    // succeeded
    int pipefd[2];
    if ( pipe(pipefd) ) {
        udi_printf("failed to create pipe: %s\n", strerror(errno));
        return -1;
    }

    if ( fcntl(pipefd[0], F_SETFD, FD_CLOEXEC) != 0 ) {
        udi_printf("failed to set close-on-exec flag on fd: %s\n",
                strerror(errno));
        return -1;
    }

    if ( fcntl(pipefd[1], F_SETFD, FD_CLOEXEC) != 0 ) { 
        udi_printf("failed to set close-on-exec flag on fd: %s\n",
                strerror(errno));
        return -1;
    }

    udi_pid child = fork();

    if ( child ) {
        // Parent
        close(pipefd[1]);

        // Wait for child to close the pipe or write errno to it to indicate
        // an error
        int errnum = 0;
        int read_result = read_all(pipefd[0], &errnum, sizeof(int));

        close(pipefd[0]);

        if ( read_result == 0 ) {
            // This indicates the child failed to exec the process
            udi_printf("child failed to execute executable %s: %s\n",
                    executable, strerror(errnum));
            strncpy(errmsg, strerror(errnum), ERRMSG_SIZE);
            return -1;
        }
        
        if ( read_result > 0 ) {
            // This means the read failed, try to clean up the child
            kill(child, SIGKILL); // explicitly ignore return value
            return -1;
        }
           
        // This means the exec succeeded because the read returned end-of-file
        return child;
    }

    // Child
    close(pipefd[0]);

    // TODO override LD_PRELOAD with rt library

    int exec_result = execve(executable, argv, envp);

    if ( exec_result == -1 ) {
        // alert the parent that the exec failed
        int errnum = errno;
        write_all(pipefd[1], &errnum, sizeof(int)); // explicity ignore errors
    }

    exit(-1);
}

static const unsigned int PID_STR_LEN = 10;

int initialize_process(udi_process *proc) 
{
    // define paths to UDI debuggee files
    size_t events_file_length = strlen(udi_root_dir) + PID_STR_LEN + DS_LEN*2 +
        strlen(EVENTS_FILE_NAME);
    char *events_file_path = (char *)malloc(events_file_length);
    snprintf(events_file_path, events_file_length, "%s/%u/%s",
            udi_root_dir, proc->pid, EVENTS_FILE_NAME);

    size_t request_file_length = strlen(udi_root_dir) + PID_STR_LEN + DS_LEN*2
        + strlen(REQUEST_FILE_NAME);
    char *request_file_path = (char *)malloc(request_file_length);
    snprintf(request_file_path, request_file_length, "%s/%u/%s",
            udi_root_dir, proc->pid, REQUEST_FILE_NAME);

    size_t response_file_length = strlen(udi_root_dir) + PID_STR_LEN + DS_LEN*2
        + strlen(RESPONSE_FILE_NAME);
    char *response_file_path = (char *)malloc(response_file_length);
    snprintf(response_file_path, response_file_length, "%s/%u/%s",
            udi_root_dir, proc->pid, RESPONSE_FILE_NAME);

    // TODO retrieve this from the debuggee
    proc->architecture = UDI_ARCH_X86;

    int errnum = 0;

    do {
        if (    events_file_path == NULL
             || request_file_path == NULL
             || response_file_path == NULL )
        {
            udi_printf("failed to allocate UDI file paths: %s\n",
                    strerror(errno));
            errnum = errno;
            break;
        }

        // poll for change in root UDI filesystem
 
        // Note: if this becomes a bottleneck non-portable approaches can be used
        // such as inotify or kqueue
        while(1) {
            struct stat events_stat;
            if ( stat(events_file_path, &events_stat) == 0 ) {
                break;
            }else{
                if ( errno == ENOENT ) {
                    // Check to make sure process did not exit
                    if ( kill(proc->pid, 0) != 0 ) {
                        udi_printf("debuggee exited before handshake: %s\n",
                                strerror(errno));
                        errnum = errno;
                        break;
                    }
                }else {
                    udi_printf("failed to wait for events file to be created: %s\n",
                            strerror(errno));
                    errnum = errno;
                    break;
                }
            }
        }

        // Cascade errors in inner loop
        if ( errnum != 0 ) break;

        // order matters here because POSIX FIFOs block in these calls

        // open request file, 
        if ( (proc->request_handle = open(request_file_path, O_WRONLY)) == -1 ) {
            udi_printf("failed to open request file %s: %s\n",
                    request_file_path, strerror(errno));
            errnum = errno;
            break;
        }

        // send init request
        udi_request init_request;
        init_request.request_type = UDI_REQ_INIT;
        init_request.length = 0;
        init_request.packed_data = NULL;

        if ( (errnum = write_request(&init_request, proc)) != 0 ) {
            udi_printf("%s\n", "failed to send init request");
            break;
        }

        // open response file, events file

        if ( (proc->response_handle = open(response_file_path, O_RDONLY)) == -1 ) {
            udi_printf("failed to open response file %s: %s\n",
                    response_file_path, strerror(errno));
            errnum = errno;
            break;
        }

        if ( (proc->events_handle = open(events_file_path, O_RDONLY)) == -1 ) {
            udi_printf("failed to open events file %s: %s\n",
                    events_file_path, strerror(errno));
            errnum = errno;
            break;
        }

        // read init response
        udi_response *init_response = NULL;
        do{ 
            init_response = read_response(proc);
            if (init_response == NULL ) {
                udi_printf("%s\n", "failed to receive init response");
                errnum = errno;
                break;
            }

            if ( init_response->response_type == UDI_RESP_ERROR ) {
                log_error_msg(init_response, __FILE__, __LINE__);
                errnum = -1;
                break;
            }

            if ( init_response->request_type != UDI_REQ_INIT ) {
                udi_printf("%s\n", "invalid init response recieved");
                errnum = 1;
                break;
            }
        }while(0);

        if (init_response != NULL ) free_response(init_response);

    }while(0);

    if ( events_file_path != NULL ) free(events_file_path);
    if ( request_file_path != NULL ) free(request_file_path);
    if ( response_file_path != NULL ) free(response_file_path);

    return errnum;
}

int write_request(udi_request *req, udi_process *proc)
{
    int errnum = 0;
    do {
        udi_request_type tmp_type = req->request_type;
        tmp_type = udi_request_type_hton(tmp_type);
        if ( (errnum = write_all(proc->request_handle, &tmp_type,
                        sizeof(udi_request_type))) != 0 ) break;

        udi_length tmp_length = req->length;
        tmp_length = udi_length_hton(tmp_length);
        if ( (errnum = write_all(proc->request_handle, &tmp_length,
                       sizeof(udi_length))) != 0 ) break;

        if ( (errnum = write_all(proc->request_handle, req->packed_data,
                        req->length)) != 0 ) break;
    }while(0);

    if ( errnum != 0 ) {
        udi_printf("failed to send request: %s\n", strerror(errnum));
    }

    return errnum;
}

udi_response *read_response(udi_process *proc) 
{
    int errnum = 0;
    udi_response *response = (udi_response *)malloc(sizeof(udi_response));

    if ( response == NULL ) {
        udi_printf("malloc failed: %s\n", strerror(errno));
        return NULL;
    }

    response->packed_data = NULL;
    do {
        // read the response type and length
        if ( (errnum = read_all(proc->response_handle, 
                        &(response->response_type), 
                        sizeof(udi_response_type))) != 0 ) break;
        response->response_type = udi_response_type_ntoh(
                response->response_type);

        if ( (errnum = read_all(proc->response_handle,
                    &(response->request_type), 
                    sizeof(udi_request_type))) != 0 ) break;
        response->request_type = udi_request_type_ntoh(response->request_type);

        if ( (errnum = read_all(proc->response_handle,
                        &(response->length), sizeof(udi_length))) != 0 ) break;
        response->length = udi_length_ntoh(response->length);

        if ( response->length > 0 ) {
            response->packed_data = malloc(response->length);
            if ( response->packed_data == NULL ) {
                errnum = errno;
                break;
            }
            if ( (errnum = read_all(proc->response_handle,
                            response->packed_data,
                            response->length)) != 0 ) break;
        }
    }while(0);

    if ( errnum != 0 ) {
        if ( errnum > 0 ) {
            udi_printf("read_all failed: %s\n", strerror(errnum));
        }
        free_response(response);
        return NULL;
    }

    return response;
}

udi_event *read_event(udi_process *proc) {
    udi_event_internal *event = (udi_event_internal *)
        malloc(sizeof(udi_event_internal));

    if ( event == NULL ) {
        udi_printf("malloc failed: %s\n", strerror(errno));
        return NULL;
    }

    // Read the raw event
    int errnum = 0;
    do {
        if ( (errnum = read_all(proc->events_handle,
                        &(event->event_type),
                        sizeof(udi_event_type))) != 0 ) break;
        event->event_type = udi_event_type_ntoh(event->event_type);

        if ( (errnum = read_all(proc->events_handle,
                        &(event->length),
                        sizeof(udi_length))) != 0 ) break;
        event->length = udi_length_ntoh(event->length);

        if (event->length > 0) {
            event->packed_data = malloc(event->length);
            if ( event->packed_data == NULL ) {
                errnum = errno;
                break;
            }

            if ( (errnum = read_all(proc->events_handle, 
                            event->packed_data,
                            event->length)) != 0 ) break;
        }
    }while(0);

    if (errnum != 0 ) {
        if ( errnum > 0 ) {
            udi_printf("read_all failed: %s\n", strerror(errnum));
        }

        free_event_internal(event);
        return NULL;
    }

    // Convert the event to a higher level representation
    udi_event *ret_event = decode_event(proc, event);
    free_event_internal(event);

    return ret_event;
}

void set_user_data(udi_process *proc, void *user_data) {
    proc->user_data = user_data;
}

void *get_user_data(udi_process *proc) {
    return proc->user_data;
}

udi_event *wait_for_events(udi_process *procs[], int num_procs) {
    fd_set read_set;
    FD_ZERO(&read_set);

    int i, max_fd = -1;
    for (i = 0; i < num_procs; ++i) {
        FD_SET(procs[i]->events_handle, &read_set);
        if (procs[i]->events_handle > max_fd ) {
            max_fd = procs[i]->events_handle;
        }
    }

    udi_event *return_list = NULL;
    udi_event *current_event = NULL;
    do {
        fd_set changed_set = read_set;
        int result = select(max_fd + 1, &changed_set, NULL, NULL, NULL);

        if ( result == -1 ) {
            if ( errno == EINTR ) {
                udi_printf("%s\n", "select() call interrupted, trying again");
                continue;
            }else{
                udi_printf("error waiting for events: %s\n", strerror(errno));
                break;
            }
        }else if ( result == 0 ) {
            udi_printf("%s\n", "select() returned 0 unexpectedly");
            break;
        }

        for (i = 0; i < num_procs; ++i) {
            if ( FD_ISSET(procs[i]->events_handle, &changed_set) ) {
                udi_event *new_event = read_event(procs[i]);

                if ( new_event == NULL ) {
                    udi_printf("failed to read event for process %d\n",
                            procs[i]->pid);
                    if ( return_list != NULL ) {
                        free_event_list(return_list);
                    }
                    return NULL;
                }

                if ( return_list == NULL ) {
                    // First event
                    return_list = new_event;
                    current_event = new_event;
                }else{
                    current_event->next_event = new_event;
                    current_event = new_event;
                }
            }
        }

        break;
    }while(1);

    return return_list;
}
