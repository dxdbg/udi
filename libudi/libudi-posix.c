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
#include <pwd.h>

const int INVALID_UDI_PID = -1;

static const char *DEFAULT_UDI_RT_LIB_NAME = "libudirt.so";

static const unsigned int PID_STR_LEN = 10;
static const unsigned int USER_STR_LEN = 25;

extern char **environ;

static
int mkdir_with_check(const char *dir) {
    if (mkdir(dir, S_IRWXG | S_IRWXU) == -1) {
        if ( errno == EEXIST ) {
            // Make sure it is a directory
            struct stat dirstat;
            if ( stat(dir, &dirstat) != 0 ) {
                udi_printf("failed to stat file %s: %s\n",
                        dir, strerror(errno));
                return -1;
            }

            if (!S_ISDIR(dirstat.st_mode)) {
                udi_printf("UDI root dir %s exists and is not a directory\n",
                        dir);
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

int create_root_udi_filesystem() {
    int result = mkdir_with_check(udi_root_dir);
    if ( result ) return result;

    uid_t user_id = geteuid();
    struct passwd *passwd_info = getpwuid(user_id);
    if ( passwd_info == NULL ) {
        udi_printf("failed to create user udi dir: %s\n",
                strerror(errno));
        return -1;
    }

    size_t user_dir_length = strlen(udi_root_dir) + DS_LEN + USER_STR_LEN + 1;
    char *user_udi_dir = (char *)malloc(sizeof(char)*user_dir_length);
    if ( user_udi_dir == NULL ) {
        udi_printf("failed to create user udi dir: %s\n",
                strerror(errno));
        return -1;
    }

    snprintf(user_udi_dir, user_dir_length, "%s/%s", udi_root_dir, 
            passwd_info->pw_name);
    result = mkdir_with_check(user_udi_dir);
    free(user_udi_dir);

    return result;
}

char * const* get_environment() {
    return environ;
}

void check_debug_logging() {
    if ( getenv(UDI_DEBUG_ENV) != NULL ) {
        udi_debug_on = 1;
        udi_printf("%s\n", "UDI lib debugging logging enabled");
    }
}

static
char **insert_rt_library(char * const envp[]) {
    // Look for the LD_PRELOAD value in the specified environment
    // If it exists, append the runtime library
    // If it does not, add it
 
    int i, ld_preload_index = -1;
    for (i = 0; envp[i] != NULL; ++i) {
        if (strncmp(envp[i],"LD_PRELOAD=", strlen("LD_PRELOAD=")) == 0) {
            ld_preload_index = i;
        }
    }

    // Allocate a local copy of the array
    int original_elements = i+1;
    int num_elements = i+1;
    if (ld_preload_index == -1) {
        num_elements++;
    }

    // Make a new copy of the array, moving the LD_PRELOAD element to the end
    char **envp_copy = (char **)malloc(num_elements*sizeof(char *));

    if ( envp_copy == NULL ) {
        udi_printf("Failed to allocate memory: %s\n", strerror(errno));
        return NULL;
    }

    int j;
    for (i = 0, j = 0; i < (original_elements-1); ++i, ++j) {
        if ( i == ld_preload_index ) {
            // Skip over this location, it will be moved to the end
            i++;
        }else{
            envp_copy[j] = envp[i];
        }
    }

    // Second to last element is the LD_PRELOAD variable
    if ( ld_preload_index == -1 ) {
        size_t str_size = strlen("LD_PRELOAD=") +
                    strlen(DEFAULT_UDI_RT_LIB_NAME) + 
                    1; // the \0 terminating character

        envp_copy[num_elements-2] = (char *)malloc(sizeof(char)*(str_size));
        memset(envp_copy[num_elements-2], 0, str_size);

        strncpy(envp_copy[num_elements-2], "LD_PRELOAD=", strlen("LD_PRELOAD="));
        strncat(envp_copy[num_elements-2], DEFAULT_UDI_RT_LIB_NAME, strlen(DEFAULT_UDI_RT_LIB_NAME));
    }else{
        size_t str_size = strlen(envp[ld_preload_index]) +
                    1 + // for : character
                    strlen(DEFAULT_UDI_RT_LIB_NAME) +
                    1; // the \0 terminating character

        envp_copy[num_elements-2] = (char *)malloc(sizeof(char)*(str_size));
        strncpy(envp_copy[num_elements-2], envp[ld_preload_index], strlen(envp[ld_preload_index]));
        strncat(envp_copy[num_elements-2], ":", 1);
        strncat(envp_copy[num_elements-2], DEFAULT_UDI_RT_LIB_NAME, strlen(DEFAULT_UDI_RT_LIB_NAME));
    }

    // Array needs to be terminated by NULL element
    envp_copy[num_elements-1] = NULL;

    return envp_copy;
}

static
void free_envp_copy(char **envp_copy) {
    // Free the allocated string for LD_PRELOAD
    char *last_element = NULL;
    int i;
    for (i = 0; envp_copy[i] != NULL; ++i) {
        last_element = envp_copy[i];
    }
    free(last_element);

    // Free the array
    free(envp_copy);
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

    // Force load the runtime library
    char **envp_copy = insert_rt_library(envp);
    if ( envp_copy == NULL ) {
        udi_printf("failed to insert runtime library\n");
        return -1;
    }

    // Use the following procedure to get feedback on whether the exec
    // succeeded
    int pipefd[2];
    if ( pipe(pipefd) ) {
        udi_printf("failed to create pipe: %s\n", strerror(errno));
        free_envp_copy(envp_copy);
        return -1;
    }

    if ( fcntl(pipefd[0], F_SETFD, FD_CLOEXEC) != 0 ) {
        udi_printf("failed to set close-on-exec flag on fd: %s\n",
                strerror(errno));
        free_envp_copy(envp_copy);
        return -1;
    }

    if ( fcntl(pipefd[1], F_SETFD, FD_CLOEXEC) != 0 ) { 
        udi_printf("failed to set close-on-exec flag on fd: %s\n",
                strerror(errno));
        free_envp_copy(envp_copy);
        return -1;
    }

    udi_pid child = fork();

    if ( child ) {
        // Parent
        free_envp_copy(envp_copy);

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

    /*
    int i;
    udi_printf("%s\n", "Environment:");
    for (i = 0; envp_copy[i] != NULL; ++i) {
        udi_printf("\t%s\n", envp_copy[i]);
    }
    */

    int exec_result = execve(executable, argv, envp_copy);

    if ( exec_result == -1 ) {
        // alert the parent that the exec failed
        int errnum = errno;
        write_all(pipefd[1], &errnum, sizeof(int)); // explicity ignore errors
    }

    exit(-1);
}


int initialize_process(udi_process *proc) 
{
    uid_t uid = geteuid();
    struct passwd *passwd_info = getpwuid(uid);

    size_t basedir_length = strlen(udi_root_dir) + PID_STR_LEN + USER_STR_LEN
        + DS_LEN*2 + 1;

    // define paths to UDI debuggee files
    size_t events_file_length = basedir_length + DS_LEN + strlen(EVENTS_FILE_NAME);
    char *events_file_path = (char *)malloc(events_file_length);
    snprintf(events_file_path, events_file_length, "%s/%s/%u/%s",
            udi_root_dir, passwd_info->pw_name, proc->pid, EVENTS_FILE_NAME);

    size_t request_file_length = basedir_length + DS_LEN + strlen(REQUEST_FILE_NAME);
    char *request_file_path = (char *)malloc(request_file_length);
    snprintf(request_file_path, request_file_length, "%s/%s/%u/%s",
            udi_root_dir, passwd_info->pw_name, proc->pid, REQUEST_FILE_NAME);

    size_t response_file_length = basedir_length + DS_LEN + strlen(RESPONSE_FILE_NAME);
    char *response_file_path = (char *)malloc(response_file_length);
    snprintf(response_file_path, response_file_length, "%s/%s/%u/%s",
            udi_root_dir, passwd_info->pw_name, proc->pid, RESPONSE_FILE_NAME);

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
                errnum = -1;
                break;
            }


            if ( udi_unpack_data(init_response->packed_data, init_response->length,
                        UDI_DATATYPE_INT32, &(proc->protocol_version),
                        UDI_DATATYPE_INT32, &(proc->architecture),
                        UDI_DATATYPE_INT32, &(proc->multithread_capable)) )
            {
                udi_printf("%s\n", "failed to unpack init response");
                errnum = -1;
                break;
            }

            if ( proc->protocol_version != UDI_PROTOCOL_VERSION ) {
                udi_printf("%s\n", "debuggee uses incompatible protocol version");
                errnum = -1;
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
    event->packed_data = NULL;

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
