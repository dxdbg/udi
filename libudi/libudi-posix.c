/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "udi.h"
#include "libudi.h"
#include "udi-common.h"
#include "libudi-private.h"
#include "udi-common-posix.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <pwd.h>
#include <inttypes.h>

const int INVALID_UDI_PID = -1;
int processes_created = 0;

static const char *DEFAULT_UDI_RT_LIB_NAME = "libudirt.so";

static const unsigned int PID_STR_LEN = 10;
static const unsigned int USER_STR_LEN = 25;
static const unsigned int TID_STR_LEN = 32;

extern char **environ;

/**
 * Writes a request to the specified fd
 *
 * @param req   the request
 * @param fd    the fd
 *
 * @return 0 on success, non-zero on failure; if result > 0, it is errno
 */
static
int write_request_fd(udi_request *req, int fd) {
    int errnum = 0;
    do {
        udi_request_type tmp_type = req->request_type;
        tmp_type = udi_request_type_hton(tmp_type);
        if ( (errnum = write_all(fd, &tmp_type,
                        sizeof(udi_request_type))) != 0 ) break;

        udi_length tmp_length = req->length;
        tmp_length = udi_length_hton(tmp_length);
        if ( (errnum = write_all(fd, &tmp_length,
                        sizeof(udi_length))) != 0 ) break;

        if ( (errnum = write_all(fd, req->packed_data,
                        req->length)) != 0 ) break;
    }while(0);

    if ( errnum != 0 ) {
        udi_printf("failed to send request: %s\n", strerror(errnum));
    }

    return errnum;
}

/**
 * Writes a request to the specified process
 *
 * @param req           the request
 * @param proc          the process handle
 *
 * @return 0 on success, non-zero on failure; if result > 0, it is errno
 */
int write_request(udi_request *req, udi_process *proc) {
    return write_request_fd(req, proc->request_handle);
}

/**
 * Writes a request to the specified thread
 *
 * @param req           the request
 * @param thr           the thread
 *
 * @return 0 on success, non-zero on failure; if result >0, it is errno
 */
int write_request_thr(udi_request *req, udi_thread *thr) {
    return write_request_fd(req, thr->request_handle);
}

/**
 * Reads a response from the specified process
 *
 * @param proc  the process handle
 *
 * @return 0 on success, non-zero on failure; if result > 0, it is errno
 */
static
udi_response *read_response_fd(int fd) 
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
        if ( (errnum = read_all(fd, 
                        &(response->response_type), 
                        sizeof(udi_response_type))) != 0 ) break;
        response->response_type = udi_response_type_ntoh(
                response->response_type);

        if ( (errnum = read_all(fd,
                    &(response->request_type), 
                    sizeof(udi_request_type))) != 0 ) break;
        response->request_type = udi_request_type_ntoh(response->request_type);

        if ( (errnum = read_all(fd,
                        &(response->length), sizeof(udi_length))) != 0 ) break;
        response->length = udi_length_ntoh(response->length);

        if ( response->length > 0 ) {
            response->packed_data = malloc(response->length);
            if ( response->packed_data == NULL ) {
                errnum = errno;
                break;
            }
            if ( (errnum = read_all(fd,
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

/**
 * Reads a response from the specified process
 *
 * @param proc  the process handle
 *
 * @return 0 on success, non-zero on failure; if result > 0, it is errno
 */
udi_response *read_response(udi_process *proc) {
    return read_response_fd(proc->response_handle);
}

/**
 * Reads a response from the specified thread
 *
 * @param thr the thread
 *
 * @return 0 on success, non-zero on failure; if result > 0, it is errno
 */
udi_response *read_response_thr(udi_thread *thr) {
    return read_response_fd(thr->response_handle);
}

/**
 * Reads an event from the specified process
 *
 * @param proc the process handle
 * @param output the output event
 *
 * @return 0 on success
 * @return < 0 when the debuggee has closed the event pipe
 * @return > 0 when the is an error reading the event from the debuggee
 */
int read_event(udi_process *proc, udi_event **output) {
    udi_event_internal *event = (udi_event_internal *)malloc(sizeof(udi_event_internal));
    event->packed_data = NULL;

    if ( event == NULL ) {
        udi_printf("malloc failed: %s\n", strerror(errno));
        return 0;
    }

    // Read the raw event
    int errnum = 0;
    do {
        if ( (errnum = read_all(proc->events_handle,
                        &(event->event_type),
                        sizeof(udi_event_type))) != 0 ) break;
        event->event_type = udi_event_type_ntoh(event->event_type);

        if ( (errnum = read_all(proc->events_handle,
                        &(event->thread_id),
                        sizeof(event->thread_id))) != 0 ) break;
        event->thread_id = udi_uint64_t_ntoh(event->thread_id);

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
        return errnum;
    }

    // Convert the event to a higher level representation
    udi_event *ret_event = decode_event(proc, event);
    free_event_internal(event);

    *output = ret_event;
    return 0;
}

/**
 * Attempts to make the specified directory with some interpretation
 * of results of the make operation. It is okay if directory already
 * exists
 *
 * @param dir   the full path to directory to create
 *
 * @return zero on success; non-zero otherwise
 */
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
                udi_printf("%s exists and is not a directory\n",
                        dir);
                return -1;
            }
            // It exists and it is a directory
        }else{
            udi_printf("error creating dir %s: %s\n", dir,
                strerror(errno));
            return -1;
        }    
    }

    return 0;
}

/**
 * Creates the root UDI filesystem for the current
 * effective user.
 *
 * This needs to be idempotent.
 *
 * @param root_dir the root directory
 * 
 * @return zero on success; non-zero otherwise
 */
int create_root_udi_filesystem(const char *root_dir) {
    int result = mkdir_with_check(root_dir);
    if ( result ) return result;

    uid_t user_id = geteuid();
    struct passwd *passwd_info = getpwuid(user_id);
    if ( passwd_info == NULL ) {
        udi_printf("failed to create user udi dir: %s\n",
                strerror(errno));
        return -1;
    }

    size_t user_dir_length = strlen(root_dir) + DS_LEN + USER_STR_LEN + 1;
    char *user_udi_dir = (char *)malloc(sizeof(char)*user_dir_length);
    if ( user_udi_dir == NULL ) {
        udi_printf("failed to create user udi dir: %s\n",
                strerror(errno));
        return -1;
    }

    snprintf(user_udi_dir, user_dir_length, "%s/%s", root_dir, 
            passwd_info->pw_name);
    result = mkdir_with_check(user_udi_dir);

    if ( result != 0 ) {
        udi_printf("failed to create root udi filesystem at %s\n",
                root_dir);
    }else{
        udi_printf("udi root filesystem located at %s\n", root_dir);
    }

    free(user_udi_dir);

    return result;
}

/**
 * @return the environment for this process
 */
char * const* get_environment() {
    return environ;
}

/**
 * Turns on debug logging if the appropriate environ. var is set
 */
void check_debug_logging() {
    if ( getenv(UDI_DEBUG_ENV) != NULL ) {
        udi_debug_on = 1;
        udi_printf("%s\n", "UDI lib debugging logging enabled");
    }
}

/**
 * Adds the UDI RT library into the LD_PRELOAD environ. var. It is
 * created at the end of the array if it does not already exist. Adds the UDI_ROOT_DIR_ENV
 * environment variable to the end of the array after LD_PRELOAD. This environment variable
 * is replaced if it already exists.
 *
 * @param the current environment
 * @param the root directory to use
 *
 * @return the new copy of the environment
 *
 * Note: the returned pointer should be free'd by the user. Additionally,
 * the user should free the last two elements of the array (LD_PRELOAD and UDI_ROOT_DIR_ENV)
 */
static
char **modify_environment(char * const envp[], const char *root_dir) {
    // Look for the LD_PRELOAD value in the specified environment
    // If it exists, append the runtime library
    // If it does not, add it
    
    const int LD_DEBUG = 0;

    char *root_dir_entry = (char *)malloc(strlen(UDI_ROOT_DIR_ENV)+1+1);
    if (root_dir_entry == NULL) {
	udi_printf("Failed to allocate memory: %s", strerror(errno));
	return NULL;
    }

    strncpy(root_dir_entry, UDI_ROOT_DIR_ENV, strlen(UDI_ROOT_DIR_ENV)+1);
    strncat(root_dir_entry, "=", 1);

    int i, ld_preload_index = -1, root_dir_index = -1;
    for (i = 0; envp[i] != NULL; ++i) {
        if (strncmp(envp[i],"LD_PRELOAD=", strlen("LD_PRELOAD=")) == 0) {
            ld_preload_index = i;
        }
        if (strncmp(envp[i], root_dir_entry, strlen(root_dir_entry)) == 0) {
            root_dir_index = i;
        }
    }
    free(root_dir_entry);

    // Allocate a local copy of the array
    int original_elements = i+1;
    int num_elements = i+1;
    if (ld_preload_index == -1) {
        num_elements++;
    }
    if (root_dir_index == -1) {
        num_elements++;
    }

    if (LD_DEBUG) {
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
        if ( i == ld_preload_index || i == root_dir_index ) {
            // Skip over this location, it will be moved to the end
            i++;
        }else{
            envp_copy[j] = envp[i];
        }
    }

    if (LD_DEBUG) {
        size_t str_size = strlen("LD_DEBUG=all") + 1;

        envp_copy[num_elements-4] = (char *)malloc(sizeof(char)*str_size);
        memset(envp_copy[num_elements-4], 0, str_size);

        strncpy(envp_copy[num_elements-4], "LD_DEBUG=all", str_size-1);
    }

    // Third to last element is the LD_PRELOAD variable
    if ( ld_preload_index == -1 ) {
        size_t str_size = strlen("LD_PRELOAD=") +
                    strlen(DEFAULT_UDI_RT_LIB_NAME) + 
                    1; // the \0 terminating character

        envp_copy[num_elements-3] = (char *)malloc(sizeof(char)*(str_size));
        memset(envp_copy[num_elements-3], 0, str_size);

        strncpy(envp_copy[num_elements-3], "LD_PRELOAD=", strlen("LD_PRELOAD="));
        strncat(envp_copy[num_elements-3], DEFAULT_UDI_RT_LIB_NAME, strlen(DEFAULT_UDI_RT_LIB_NAME));
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

    // Second to last element is the UDI_ROOT_DIR_ENV variable
    size_t str_size = strlen(UDI_ROOT_DIR_ENV) +
        1 + // = 
        strlen(root_dir) +
        1; // the \0 terminating character

    envp_copy[num_elements-2] = (char *)malloc(sizeof(char)*(str_size));
    memset(envp_copy[num_elements-2], 0, str_size);

    strncpy(envp_copy[num_elements-2], UDI_ROOT_DIR_ENV, strlen(UDI_ROOT_DIR_ENV));
    strncat(envp_copy[num_elements-2], "=", 1);
    strncat(envp_copy[num_elements-2], root_dir, strlen(root_dir));

    // Array needs to be terminated by NULL element
    envp_copy[num_elements-1] = NULL;

    return envp_copy;
}

/**
 * Frees the array created by the modify_environment function
 *
 * @param envp_copy     the array to free
 */
static
void free_envp_copy(char **envp_copy) {
    // Free the allocated string for LD_PRELOAD and UDI_ROOT_DIR_ENV
    int i;
    for (i = 0; envp_copy[i] != NULL; ++i);
    free(envp_copy[i-2]); // UDI_ROOT_DIR_ENV
    free(envp_copy[i-1]); // LD_PRELOAD

    // Free the array
    free(envp_copy);
}

/**
 * Forks a new process and executes the specified executable, arguments
 * and environment
 *
 * @param proc          the process
 * @param executable    the executable to exec
 * @param argv          the arguments to the new process
 * @param envp          the environment for the new process
 */
udi_pid fork_process(udi_process *proc, const char *executable, char * const argv[],
        char * const envp[])
{
    // First process initialization
    if (processes_created == 0) { // TODO: this needs to be thread-safe
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

    if ( create_root_udi_filesystem(proc->root_dir) != 0 ) {
        udi_printf("%s\n", "failed to create root UDI filesystem");
        return -1;
    }

    if ( envp == NULL ) {
        // Created process will inherit the current environment
        envp = get_environment();
    }

    // Force load the runtime library and specify location of root udi filesystem
    char **envp_copy = modify_environment(envp, proc->root_dir);
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
            strncpy(proc->errmsg.msg, strerror(errnum), proc->errmsg.size);
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

/**
 * Initializes the specified process. This includes created all file descriptors
 * for files in the UDI filesystem as well as performing the initial handshake
 * with the debuggee
 *
 * @param proc the process handle
 *
 * @return 0 on success, non-zero on failure; if result > 0, it is errno
 */
int initialize_process(udi_process *proc) {
    uid_t uid = geteuid();
    struct passwd *passwd_info = getpwuid(uid);

    size_t basedir_length = strlen(proc->root_dir) + PID_STR_LEN + USER_STR_LEN
        + DS_LEN*2 + 1;

    // define paths to UDI debuggee files
    size_t events_file_length = basedir_length + DS_LEN + strlen(EVENTS_FILE_NAME);
    char *events_file_path = (char *)malloc(events_file_length);
    snprintf(events_file_path, events_file_length, "%s/%s/%u/%s",
            proc->root_dir, passwd_info->pw_name, proc->pid, EVENTS_FILE_NAME);

    size_t request_file_length = basedir_length + DS_LEN + strlen(REQUEST_FILE_NAME);
    char *request_file_path = (char *)malloc(request_file_length);
    snprintf(request_file_path, request_file_length, "%s/%s/%u/%s",
            proc->root_dir, passwd_info->pw_name, proc->pid, REQUEST_FILE_NAME);

    size_t response_file_length = basedir_length + DS_LEN + strlen(RESPONSE_FILE_NAME);
    char *response_file_path = (char *)malloc(response_file_length);
    snprintf(response_file_path, response_file_length, "%s/%s/%u/%s",
            proc->root_dir, passwd_info->pw_name, proc->pid, RESPONSE_FILE_NAME);

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
        uint64_t tid;
        udi_response *init_response = NULL;
        do{ 
            init_response = read_response(proc);
            if (init_response == NULL ) {
                udi_printf("%s\n", "failed to receive init response");
                errnum = errno;
                break;
            }

            if ( init_response->response_type == UDI_RESP_ERROR ) {
                log_error_msg(proc, init_response, __FILE__, __LINE__);
                errnum = -1;
                break;
            }

            if ( init_response->request_type != UDI_REQ_INIT ) {
                udi_printf("%s\n", "invalid init response recieved");
                errnum = -1;
                break;
            }

            if ( unpack_response_init(init_response,
                        &(proc->protocol_version),
                        &(proc->architecture),
                        &(proc->multithread_capable),
                        &tid) )
            {

                udi_printf("%s\n", "failed to unpack init response");
                errnum = -1;
                break;
            }

            if ( proc->protocol_version != UDI_PROTOCOL_VERSION_1 ) {
                udi_printf("%s\n", "debuggee uses incompatible protocol version");
                errnum = -1;
                break;
            }

            udi_thread *thr;
            if ( (thr = handle_thread_create(proc, tid)) == NULL ) {
                udi_printf("%s\n", "failed to handle creation of initial thread");
                errnum = -1;
                break;
            }

            udi_printf("completed initialization of initial thread 0x%"PRIx64"\n",
                    thr->tid);
        }while(0);

        if (init_response != NULL ) free_response(init_response);
    }while(0);

    if ( events_file_path != NULL ) free(events_file_path);
    if ( request_file_path != NULL ) free(request_file_path);
    if ( response_file_path != NULL ) free(response_file_path);

    if ( errnum == 0 ) {
        // TODO: this needs to be thread-safe
        processes_created++;
    }

    return errnum;
}

/**
 * Creates a thread structure for the specified tid
 *
 * @param proc  the process in which the thread was created
 * @param tid   the thread id
 *
 * @return the created thread structure or NULL if no thread could be created
 */
udi_thread *handle_thread_create(udi_process *proc, uint64_t tid) {

    udi_thread *thr = (udi_thread *)malloc(sizeof(udi_thread));
    if (thr == NULL) {
        udi_printf("malloc failed: %s\n", strerror(errno));
        return NULL;
    }
    memset(thr, 0, sizeof(udi_thread));
    thr->tid = tid;
    thr->proc = proc;
    thr->next_thread = NULL;
    thr->state = UDI_TS_RUNNING;
    thr->single_step = 0;

    // perform the handshake with the debuggee //
    
    // define paths to UDI debuggee files
    uid_t uid = geteuid();
    struct passwd *passwd_info = getpwuid(uid);
    size_t basedir_length = strlen(proc->root_dir) + PID_STR_LEN + USER_STR_LEN
        + TID_STR_LEN + DS_LEN*3 + 1;

    size_t request_file_length = basedir_length + DS_LEN + strlen(REQUEST_FILE_NAME);
    char *request_file_path = (char *)malloc(request_file_length);
    snprintf(request_file_path, request_file_length, "%s/%s/%u/%"PRIx64"/%s",
            proc->root_dir, passwd_info->pw_name, proc->pid, thr->tid, REQUEST_FILE_NAME);

    size_t response_file_length = basedir_length + DS_LEN + strlen(RESPONSE_FILE_NAME);
    char *response_file_path = (char *)malloc(response_file_length);
    snprintf(response_file_path, response_file_length, "%s/%s/%u/%"PRIx64"/%s",
            proc->root_dir, passwd_info->pw_name, proc->pid, thr->tid, RESPONSE_FILE_NAME);

    int result = 0;
    do {
        if ( request_file_path == NULL || response_file_path == NULL ) {
            udi_printf("%s\n", "failed to allocate thread-specific file names");
            result = -1;
            break;
        }

        // open request file 
        if ( (thr->request_handle = open(request_file_path, O_WRONLY)) == -1 ) {
            udi_printf("failed to open request file %s: %s\n",
                    request_file_path, strerror(errno));
            result = errno;
            break;
        }

        // send init request
        udi_request init_request;
        init_request.request_type = UDI_REQ_INIT;
        init_request.length = 0;
        init_request.packed_data = NULL;

        if ( (result = write_request_fd(&init_request, thr->request_handle)) != 0 ) {
            udi_printf("%s\n", "failed to send init request");
            break;
        }

        // open response file
        if ( (thr->response_handle = open(response_file_path, O_RDONLY)) == -1 ) {
            udi_printf("failed to open response file %s: %s\n",
                    response_file_path, strerror(errno));
            result = errno;
            break;
        }

        // read init response
        udi_response *init_response = NULL;
        init_response = read_response_fd(thr->response_handle);
        if (init_response == NULL ) {
            udi_printf("%s\n", "failed to receive init response");
            result = errno;
            break;
        }

        if ( init_response->response_type == UDI_RESP_ERROR ) {
            log_error_msg(proc, init_response, __FILE__, __LINE__);
            result = -1;
            break;
        }

        if ( init_response->request_type != UDI_REQ_INIT ) {
            udi_printf("%s\n", "invalid init response recieved");
            result = -1;
            break;
        }

        // link the thread into process thread list
        udi_thread *iter = proc->threads;
        while (iter != NULL && iter->next_thread != NULL) {
            iter = iter->next_thread;
        }

        if (iter == NULL) {
            proc->threads = thr;
            thr->initial = 1;
        }else{
            iter->next_thread = thr;
            thr->initial = 0;
        }
    }while(0);

    if ( result != 0 ) {
        free(response_file_path);
        free(request_file_path);
        free(thr);
        
        return NULL;
    }

    udi_printf("completed handshake for thread 0x%"PRIx64"\n", tid);

    return thr;
}

/**
 * Handles the death of a thread
 *
 * @param proc  the process handle
 * @param thr   the thread handle
 *
 * @return 0 on success; non-zero otherwise
 */
int handle_thread_death(udi_process *proc, udi_thread *thr) {

    // close all handles open to thread
    if ( close(thr->response_handle) != 0 ) {
        udi_printf("failed to close response handle for 0x%"PRIx64": %s\n",
                thr->tid, strerror(errno));
        return -1;
    }

    if ( close(thr->request_handle) != 0 ) {
        udi_printf("failed to close request handle for 0x%"PRIx64": %s\n",
                thr->tid, strerror(errno));
        return -1;
    }

    return 0;
}

udi_event *wait_for_events(udi_process *procs[], int num_procs) {
    fd_set read_set;
    FD_ZERO(&read_set);

    if (num_procs <= 0) {
        udi_printf("%s\n", "No processes specified, cannot wait for events");
        return NULL;
    }

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
                udi_event *new_event = NULL;
                int read_result = read_event(procs[i], &new_event);
                if (read_result < 0) {
                    udi_printf("process %d has closed its event pipe\n", procs[i]->pid);
                    new_event = (udi_event *)malloc(sizeof(udi_event));
                    if (new_event == NULL) {
                        udi_printf("%s\n", "failed to allocate event");
                        free_event_list(return_list);
                        return NULL;
                    }
                    new_event->event_type = UDI_EVENT_PROCESS_CLEANUP;
                    new_event->thr = get_initial_thread(procs[i]);
                    new_event->proc = procs[i];
                    new_event->event_data = NULL;
                    new_event->next_event = NULL;

                    procs[i]->terminated = 1;
                }else if (read_result > 0) {
                    udi_printf("failed to read event for process %d\n",
                            procs[i]->pid);
                    free_event_list(return_list);
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

                // The process is no longer running
                udi_printf("process %d has stopped due to an event\n", procs[i]->pid);
                procs[i]->running = 0;
            }
        }

        break;
    }while(1);

    return return_list;
}
