/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// UDI implementation specific to POSIX-compliant operating systems

// This needs to be included first because it sets feature macros
#include "udirt-platform.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <inttypes.h>

#include "udi.h"
#include "udirt.h"
#include "udirt-posix.h"

// macros
#define CASE_TO_STR(x) case x: return #x

// testing
extern int testing_udirt(void) UDI_WEAK;

// constants
const char * const UDI_DS = "/";
const unsigned int DS_LEN = 1;
const char * const DEFAULT_UDI_ROOT_DIR = "/tmp/udi";
static const unsigned int PID_STR_LEN = 10;

// file paths
static char *basedir_name;
static char *requestfile_name;
static char *responsefile_name;
static char *eventsfile_name;

// file handles
static udirt_fd request_handle = -1;
static udirt_fd response_handle = -1;
udirt_fd events_handle = -1;

// write/read permission fault handling
static int failed_si_code = 0;

// write failure handling
int pipe_write_failure = 0;

// Continue handling
static int pass_signal = 0;

// Used to force threads into the library signal handler
extern int pthread_kill(pthread_t, int) __attribute__((weak));

// unexported prototypes
static int remove_udi_filesystem();

/**
 * Disables this library
 */
static void disable_debugging() {
    udi_enabled = 0;

    // Replace the library signal handler with the application signal handlers
    if ( uninstall_signal_handlers() ) {
        udi_log("failed to uninstall signal handlers");
    }

    udi_log("Disabled debugging");
}

int single_thread_executing() {
    return (is_performing_mem_access() || continue_bp != NULL);
}

/**
 * A UDI version of abort that performs some cleanup before calling abort
 */
void udi_abort_file_line(const char *file, unsigned int line) {
    udi_log("udi_abort at %s[%d]", file, line);

    if ( real_sigaction ) {
        struct sigaction default_action;
        memset(&default_action, 0, sizeof(struct sigaction));
        default_action.sa_handler = SIG_DFL;

        // Need to avoid any user handlers
        real_sigaction(SIGABRT, (struct sigaction *)&default_action, NULL);

        disable_debugging();
    }

    abort();
}

/**
 * Translates the failed memory access code into an error
 * message
 *
 * @return the error message
 */
const char *get_mem_errstr() {
    switch(failed_si_code) {
        case SEGV_MAPERR:
            return "address not mapped in process";
        default:
            return "unknown memory error";
    }
}

void post_continue_hook(uint32_t sig_val) {
    if (!exiting) {
        pass_signal = sig_val;
        kill(getpid(), sig_val);

        udi_log("continuing with signal %d", sig_val);
    }else{
        int remove_result = remove_udi_filesystem();
        if (remove_result != 0) {
            if (remove_result > 0) {
                udi_log("failed to cleanup udi filesystem: %e", remove_result);
            }else{
                udi_log("failed to cleanup udi filesystem");
            }
        }
    }
}

/**
 * Implementation of pre_mem_access hook. Unblocks memory access related signals.
 *
 * @return the original signal set before the memory signals are blocked.
 */
void *pre_mem_access_hook() {
    // Unblock some signals to allow the write to complete
    sigset_t *original_set = (sigset_t *)udi_malloc(sizeof(sigset_t));

    if ( original_set == NULL ) {
        udi_log("failed to allocate sigset_t: %e", errno);
        return NULL;
    }

    int result = 0;
    do {
        if ( setsigmask(SIG_BLOCK, NULL, original_set) != 0 ) {
            udi_log("failed to change signal mask: %e", errno);
            result = -1;
            break;
        }

        sigset_t segv_set = *original_set;
        sigdelset(&segv_set, SIGSEGV);
        sigdelset(&segv_set, SIGBUS);
        if ( setsigmask(SIG_SETMASK, &segv_set, original_set) != 0 ) {
            udi_log("failed to unblock fault signals: %e", errno);
            result = -1;
            break;
        }
    }while(0);

    if ( result ) {
        udi_free(original_set);
        original_set = NULL;
    }

    return original_set;
}

/**
 * Implementation of post mem access hook
 *
 * @param hook_arg the value returned from the pre mem access hook
 *
 * @return 0 on success; false otherwise
 */
int post_mem_access_hook(void *hook_arg) {

    sigset_t *original_set = (sigset_t *)hook_arg;

    int result = 0;
    if ( setsigmask(SIG_SETMASK, original_set, NULL) != 0 ) {
        udi_log("failed to reset signal mask: %e", errno);
        result = -1;
    }

    udi_free(original_set);

    return result;
}

/**
 * Blocks for a request from all possible request handles
 *
 * @param thr the output parameter for the thread -- set to NULL if request is for process
 *
 * @return the result
 */
static
int block_for_request(thread **thr) {
    int max_fd = request_handle;

    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(request_handle, &read_set);

    thread *iter = get_thread_list();
    while (iter != NULL) {
        if (!iter->dead) {
            FD_SET(iter->request_handle, &read_set);
            if ( iter->request_handle > max_fd ) {
                max_fd = iter->request_handle;
            }
        }
        iter = iter->next_thread;
    }

    do {
        fd_set changed_set = read_set;
        int result = select(max_fd + 1, &changed_set, NULL, NULL, NULL);

        if ( result == -1 ) {
            if ( errno == EINTR ) {
                udi_log("select call interrupted, trying again");
                continue;
            }
            udi_log("failed to wait for request: %e", errno);
        }else if ( result == 0 ) {
            udi_log("select unexpectedly returned 0");
        }else{
            if ( FD_ISSET(request_handle, &changed_set) ) {
                *thr = NULL;
                return RESULT_SUCCESS;
            }
            iter = get_thread_list();
            while (iter != NULL) {
                if ( FD_ISSET(iter->request_handle, &changed_set) ) {
                    *thr = iter;
                    return RESULT_SUCCESS;
                }
                iter = iter->next_thread;
            }
        }
        break;
    }while(1);

    return RESULT_ERROR;
}

int wait_and_execute_command(udi_errmsg *errmsg, thread **thr) {
    int result = 0;

    int more_reqs = 1;
    while(more_reqs) {
        udi_log("waiting for request");
        result = block_for_request(thr);
        if ( result != 0 ) {
            udi_set_errmsg(errmsg, "failed to wait for request");
            udi_log("failed to wait for request");
            result = RESULT_ERROR;
            break;
        }

        udi_request_type_e type = UDI_REQ_INVALID;
        if ( *thr == NULL ) {
            udi_log("received process request");
            result = handle_process_request(request_handle,
                                            response_handle,
                                            &type,
                                            errmsg);
        }else{
            udi_log("received request for thread %a", (*thr)->id);
            result = handle_thread_request((*thr)->request_handle,
                                           (*thr)->response_handle,
                                           *thr,
                                           &type,
                                           errmsg);
        }

        if ( result != RESULT_SUCCESS ) {
            if ( result == RESULT_FAILURE ) {
                more_reqs = 1;
            }else if ( result == RESULT_ERROR ) {
                more_reqs = 0;
            }
        }else{
            more_reqs = 1;
        }

        if ( type == UDI_REQ_CONTINUE ) {
            more_reqs = 0;
        }
    }

    return result;
}

/**
 * Decodes the memory access signal event using the specified arguments
 *
 * @param sig the signal that caused the event
 * @param siginfo the siginfo argument passed to the signal handler
 * @param context the context argument passed to the signal handler
 * @param errmsg the errmsg populated on error
 *
 * @return the result of decoding the event
 */
static int decode_memory_access_event(int sig,
                                       const siginfo_t *siginfo,
                                       ucontext_t *context,
                                       int *wait_for_request,
                                       udi_errmsg *errmsg)
{
    int result;
    if ( is_performing_mem_access() ) {
        *wait_for_request = 0;

        // if the error was due to permissions, change permissions temporarily
        // to allow mem access to complete
        if (siginfo->si_code == SEGV_ACCERR) {
            unsigned long mem_access_addr_ul = (unsigned long)get_mem_access_addr();

            // page align
            mem_access_addr_ul = mem_access_addr_ul & ~(sysconf(_SC_PAGESIZE)-1);

            // Note: this could cause unwanted side effects in the debuggee
            // because it could mask errors that the debuggee should encounter.
            // However, in order to handle this case, the protection for a page
            // must be queried from the OS, and currently there is not a
            // portable way to do this. Thus, the tradeoff was made in favor of
            // portability.
            if ( mprotect((void *)mem_access_addr_ul, get_mem_access_size(),
                        PROT_READ | PROT_WRITE | PROT_EXEC) != 0 )
            {
                result = RESULT_ERROR;
                udi_set_errmsg(errmsg,
                               "failed to modify permissions for memory access: %e",
                               errno);
                udi_log("%s", errmsg->msg);
            } else {
                result = RESULT_SUCCESS;
            }
        }else{
            udi_log("failed to handle event at address %a for process %d: sig=%d, si_code=%d",
                    (uint64_t)siginfo->si_addr,
                    getpid(),
                    sig,
                    siginfo->si_code);
            result = RESULT_ERROR;
        }

        if (result != RESULT_SUCCESS) {
            set_pc(context, abort_mem_access());

            failed_si_code = siginfo->si_code;
        }

        // If failures occurred, errors will be reported by code performing mem access
        result = RESULT_SUCCESS;
    }else{
        // TODO create event and send to debugger
        udi_log("segv at address %a, pc = %a",
                (uint64_t)siginfo->si_addr,
                get_pc(context));

        result = RESULT_SUCCESS;
    }

    return result;
}

/**
 * Decodes the trap
 *
 * @param thr the current thread
 * @param siginfo the siginfo passed to the signal handler
 * @param context the context passed to the signal handler
 * @param errmsg the error message populated on error
 */
static
int decode_trap(thread *thr,
                const siginfo_t *siginfo,
                ucontext_t *context,
                int *wait_for_request,
                udi_errmsg *errmsg)
{
    uint64_t trap_address = get_trap_address(context);

    // Check and see if it corresponds to a breakpoint
    breakpoint *bp = find_breakpoint(trap_address);

    int result;
    if ( bp != NULL ) {
        udi_log("breakpoint hit at %a", trap_address);
        result = decode_breakpoint(thr, bp, context, wait_for_request, errmsg);
    }else{
        // TODO create signal event
        udi_set_errmsg(errmsg,
                       "failed to decode trap event at %a",
                       trap_address);
        udi_abort();
        result = RESULT_ERROR;
        *wait_for_request = 0;
    }

    return result;
}

/**
 * The signal handler entry point for the library
 *
 * See manpage for sigaction
 */
static
void signal_entry_impl(int signal, siginfo_t *siginfo, void *v_context) {
    udi_errmsg errmsg;
    errmsg.size = ERRMSG_SIZE;
    errmsg.msg[ERRMSG_SIZE-1] = '\0';

    ucontext_t *context = (ucontext_t *)v_context;

    if ( pipe_write_failure && signal == SIGPIPE ) {
        udi_log("Ignoring SIGPIPE due to previous library write failure");
        pipe_write_failure = 0;

        app_signal_handler(signal, siginfo, v_context);
        return;
    }

    if ( pass_signal != 0 ) {
        app_signal_handler(signal, siginfo, v_context);
        pass_signal = 0;
        return;
    }

    if (!udi_enabled && !is_performing_mem_access()) {
        udi_log("UDI disabled, not handling signal %d at addr %a",
                signal,
                (uint64_t)siginfo->si_addr);
        return;
    }

    udi_log(">>> signal entry for %a/%a with %d at %a",
               get_user_thread_id(),
               get_kernel_thread_id(),
               signal,
               get_pc(context));
    if ( is_performing_mem_access() ) {
        udi_log("memory access at %a in progress", (uint64_t)get_mem_access_addr());
    }

    thread *thr = get_current_thread();

    if ( signal == THREAD_SUSPEND_SIGNAL && (thr == NULL || thr->suspend_pending == 1) ) {
        udi_log("ignoring extraneous suspend signal for %a/%a",
                get_user_thread_id(),
                get_kernel_thread_id());

        if ( thr != NULL ) thr->suspend_pending = 0;
        return;
    }

    if ( !single_thread_executing() ) {
        // save this signal event state to allow another thread to pass control to this
        // thread later on
        thr->event_state.signal = signal;
        thr->event_state.siginfo = *siginfo;
        copy_context(context, &(thr->event_state));
        thr->event_state.context_valid = 1;

        int block_result = block_other_threads();
        if ( block_result == -1 ) {
            udi_log("failed to block other threads");
            udi_abort();
            return;
        }

        if ( block_result > 0 ) {
            thr->event_state.context_valid = 0;

            udi_log("<<< waiting thread %a exiting signal handler",
                    get_user_thread_id());
            return;
        }
    }else if (continue_bp != NULL) {
        if (thr != NULL ) {
            copy_context(context, &(thr->event_state));
            thr->event_state.context_valid = 1;
        }
    }

    udi_log("thread %a/%a now handling signal %d",
            get_user_thread_id(),
            get_kernel_thread_id(),
            signal);
    if ( thr != NULL ) {
        thr->event_state.signal = 0;
    }

    // handle the event
    int wait_for_request = 1;
    int request_error = 0;
    thread *request_thr = NULL;
    int result;
    do {
        switch(signal) {
            case SIGBUS:
            case SIGSEGV:
                result = decode_memory_access_event(signal,
                                                    siginfo,
                                                    context,
                                                    &wait_for_request,
                                                    &errmsg);
                break;
            case SIGTRAP:
                result = decode_trap(thr, siginfo, context, &wait_for_request, &errmsg);
                break;
            default:
                result = handle_unknown_event(get_user_thread_id(), &errmsg);
                break;
        }

        if ( thr != NULL && thr->single_step ) {
            uint64_t pc = get_pc(context);
            uint64_t successor = get_ctf_successor(pc, &errmsg, context);
            if (successor == 0) {
                udi_log("failed to determine successor for instruction at %a",
                        pc);
                result = RESULT_ERROR;
            }else{
                breakpoint *existing_bp = find_breakpoint(successor);

                // If it is a continue breakpoint, it will be used as a single step breakpoint. If it
                // is an event breakpoint or user breakpoint, no single step event will be
                // generated
                if ( existing_bp == NULL ) {
                    thr->single_step_bp = create_breakpoint(successor);
                    if (thr->single_step_bp == NULL) {
                        udi_log("failed to create single step breakpoint at %a",
                                successor);
                        result = RESULT_ERROR;
                    }else{
                        thr->single_step_bp->thread = thr;
                        int install_result = install_breakpoint(thr->single_step_bp, &errmsg);
                        if (install_result != 0) {
                            udi_log("failed to install single step breakpoint at %a",
                                    successor);
                            result = RESULT_ERROR;
                        }
                    }
                }
            }
        }

        if ( result != RESULT_SUCCESS ) {

            // Don't report any more errors after this event
            result = RESULT_SUCCESS;

            int error_result = handle_error_event(get_user_thread_id(), &errmsg);
            if (error_result != RESULT_SUCCESS) {
                udi_log("aborting due to event reporting failure: %s", errmsg.msg);
                udi_abort();
            }
        }

        // wait for command
        if ( wait_for_request ) {
            result = wait_and_execute_command(&errmsg, &request_thr);
            if (result == RESULT_SUCCESS) {
                request_error = 0;
            }else if (result == RESULT_FAILURE) {
                request_error = 0;
            }else if (result == RESULT_ERROR) {
                request_error = 1;
            }
        }
    }while(0);

    if ( result != RESULT_SUCCESS ) {
        if ( request_error ) {
            udi_log("Aborting due to request failure: %s", errmsg.msg);
            udi_abort();
        }
    }else{
        // Cleanup before returning to user code
        if ( !single_thread_executing() ) {
            release_other_threads();
        }
    }

    udi_log("<<< signal exit for %a/%a with %d at %a",
            get_user_thread_id(),
            get_kernel_thread_id(),
            signal,
            get_pc(context));

    if (thr != NULL) {
        thr->event_state.context_valid = 0;
    }
}

void signal_entry_point(int signal, siginfo_t *siginfo, void *v_context) {
    int saved_errno = errno;
    signal_entry_impl(signal, siginfo, v_context);
    errno = saved_errno;
}

/**
 * Enables debug logging if the environment specifies it
 */
static void enable_debug_logging() {
    // turn on debugging, if necessary
    if (getenv(UDI_DEBUG_ENV) != NULL) {
        udi_debug_on = 1;
    }
}

/**
 * Performs any necessary global initialization
 */
static void global_initialization() {
    init_thread_support();
    init_req_handling();
}

/**
 * Removes the udi filesystem for this process on exit
 */
static
int remove_udi_filesystem() {
    // TODO

    return 0;
}

/**
 * Creates the necessary elements for the udi filesystem
 */
static int create_udi_filesystem() {
    int errnum = 0;

    do {
        // get root directory
        if ((UDI_ROOT_DIR = getenv(UDI_ROOT_DIR_ENV)) == NULL) {
            UDI_ROOT_DIR = (char *)udi_malloc(strlen(DEFAULT_UDI_ROOT_DIR)+1);
            strncpy(UDI_ROOT_DIR, DEFAULT_UDI_ROOT_DIR,
                    strlen(DEFAULT_UDI_ROOT_DIR)+1);
        }

        // create the directory for this process
        size_t basedir_length = strlen(UDI_ROOT_DIR) + PID_STR_LEN
            + 1*DS_LEN + 1;
        basedir_name = (char *)udi_malloc(basedir_length);
        if (basedir_name == NULL) {
            udi_log("malloc failed: %e", errno);
            errnum = errno;
            break;
        }

        udi_formatted_str(basedir_name,
                          basedir_length,
                          "%s/%d",
                          UDI_ROOT_DIR,
                          getpid());
        if (mkdir(basedir_name, S_IRWXG | S_IRWXU) == -1) {
            udi_log("error creating basedir '%s': %e",
                    basedir_name,
                    errno);
            errnum = errno;
            break;
        }

        // create the udi files
        size_t requestfile_length = basedir_length
                                    + strlen(REQUEST_FILE_NAME) + DS_LEN;
        requestfile_name = (char *)udi_malloc(requestfile_length);
        if (requestfile_name == NULL) {
            udi_log("malloc failed: %e", errno);
            errnum = errno;
            break;
        }

        udi_formatted_str(requestfile_name,
                          requestfile_length,
                          "%s/%d/%s",
                          UDI_ROOT_DIR,
                          getpid(),
                          REQUEST_FILE_NAME);
        if (mkfifo(requestfile_name, S_IRWXG | S_IRWXU) == -1) {
            udi_log("error creating request file fifo: %e", errno);
            errnum = errno;
            break;
        }

        size_t responsefile_length = basedir_length +
                               strlen(RESPONSE_FILE_NAME) + DS_LEN;
        responsefile_name = (char *)udi_malloc(responsefile_length);
        if (responsefile_name == NULL) {
            udi_log("malloc failed: %s", errno);
            errnum = errno;
            break;
        }

        udi_formatted_str(responsefile_name,
                          responsefile_length,
                          "%s/%d/%s",
                          UDI_ROOT_DIR,
                          getpid(),
                          RESPONSE_FILE_NAME);
        if (mkfifo(responsefile_name, S_IRWXG | S_IRWXU) == -1) {
            udi_log("error creating response file fifo: %e", errno);
            errnum = errno;
            break;
        }

        size_t eventsfile_length = basedir_length +
                             strlen(EVENTS_FILE_NAME) + DS_LEN;
        eventsfile_name = (char *)udi_malloc(eventsfile_length);
        if (eventsfile_name == NULL) {
            udi_log("malloc failed: %e", errno);
            errnum = errno;
            break;
        }

        udi_formatted_str(eventsfile_name,
                          eventsfile_length,
                          "%s/%d/%s",
                          UDI_ROOT_DIR,
                          getpid(),
                          EVENTS_FILE_NAME);
        if (mkfifo(eventsfile_name, S_IRWXG | S_IRWXU) == -1) {
            udi_log("error creating event file fifo: %e", errno);
            errnum = errno;
            break;
        }
    }while(0);

    return errnum;
}

udi_barrier thread_barrier = { 0, -1, -1 };

int initialize_thread_sync() {
    thread_barrier.sync_var = 0;

    if ( get_multithread_capable() ) {
        int control_pipe[2];
        if ( pipe(control_pipe) != 0 ) {
            udi_log("failed to create sync pipe for barrier: %e", errno);
            return -1;
        }

        thread_barrier.read_handle = control_pipe[0];
        thread_barrier.write_handle = control_pipe[1];
    }else{
        thread_barrier.read_handle = -1;
        thread_barrier.write_handle = -1;
    }

    return 0;
}

const unsigned char sentinel = 0x21;

static
void read_sentinel(int fd) {
    int complete = 0;
    while (!complete) {
        unsigned char trigger;
        int result = read(fd, &trigger, 1);
        if (result == 1) {
            if (trigger != sentinel) {
                udi_abort();
                return;
            }
            complete = 1;
        } else if (result == 0) {
            udi_log("sentinel read encountered eof");
            udi_abort();
            return;
        } else if (result == -1) {
            if (errno == EINTR) continue;
            udi_log("sentinel read encountered error %e", errno);
            udi_abort();
            return;
        }
    }
}

int block_other_threads() {
    int result = 0;

    if (get_multithreaded()) {
        thread *thr = get_current_thread();
        if ( thr == NULL ) {
            udi_log("found unknown thread %a", get_user_thread_id());
            return -1;
        }

        // Determine whether this is the first thread or not
        unsigned int sync_var = __sync_val_compare_and_swap(&(thread_barrier.sync_var), 0, 1);
        if (sync_var == 0) {
            thr->control_thread = 1;
            udi_log("thread %a is the control thread", thr->id);

            int num_suspended = 0;
            thread *iter = get_thread_list();
            while (iter != NULL) {
                if ( iter != thr ) {
                    iter->control_thread = 0;

                    // only force running threads into the signal handler, suspended threads are
                    // already in the signal handler
                    if (iter->ts != UDI_TS_SUSPENDED) {
                        int kill_result = pthread_kill((pthread_t)iter->id, THREAD_SUSPEND_SIGNAL);
                        if ( kill_result != 0 ) {
                            udi_log("failed to send signal to %a: %e", iter->id, kill_result);
                            udi_abort();
                            return -1;
                        }
                        udi_log("thread %a sent suspend signal %d to thread %a",
                                thr->id,
                                THREAD_SUSPEND_SIGNAL,
                                iter->id);
                        num_suspended++;
                    }else{
                        udi_log("thread %a already suspended, not signaling",
                                iter->id);
                    }
                }
                iter = iter->next_thread;
            }

            __sync_synchronize(); // global state updated, issue full memory barrier

            // wait for the other threads to reach this function
            int i;
            for (i = 0; i < num_suspended; ++i) {
                read_sentinel(thread_barrier.read_handle);
            }
            udi_log("thread %a blocked other threads", thr->id);
        }else{
            // signal that thread is now waiting to be released
            if ( write(thread_barrier.write_handle, &sentinel, 1) != 1 ) {
                udi_log("failed to write trigger to pipe: %a", errno);
                udi_abort();
                return -1;
            }

            udi_log("thread %a waiting to be released (pending signal = %d)",
                    thr->id,
                    thr->event_state.signal);

            // Indicate that another suspend signal is pending if the reason for the thread
            // to enter the handler was not triggered by the library
            if ( thr->event_state.signal != THREAD_SUSPEND_SIGNAL ) {
                thr->suspend_pending = 1;
                udi_log("thread %a has a suspend signal pending", thr->id);
            }else{
                thr->suspend_pending = 0;
            }

            read_sentinel(thr->control_read);
        }

        // if this thread has become the control thread, act accordingly
        result = (thr->control_thread == 1) ? 0 : 1;
    }

    return result;
}

int release_other_threads() {
    if (get_multithread_capable()) {
        thread *thr = get_current_thread();
        // it is okay if this thr is NULL -- this occurs when a thread hits the death breakpoint

        thread *iter = get_thread_list();
        // determine if an event for another thread is pending
        while (iter != NULL) {
            // XXX: there is a known race here where a thread is sent a THREAD_SUSPEND_SIGNAL by
            // another source external to the library but at the same time, the library sends the same
            // signal to the thread. The result is that there is no way to handle the externally
            // sourced signal.
            if ( iter != thr &&
                 ( (iter->event_state.signal != THREAD_SUSPEND_SIGNAL &&
                    iter->event_state.signal != 0) ||
                   iter->stack_event_pending) )
            {
                break;
            }
            iter = iter->next_thread;
        }

        if ( iter != thr && iter != NULL ) {
            // Found another event that needs to be handled

            if ( thr != NULL ) {
                __sync_val_compare_and_swap(&(thr->control_thread), 1, 0);
            }

            __sync_val_compare_and_swap(&(iter->control_thread), 0, 1);

            if ( write(iter->control_write, &sentinel, 1) != 1 ) {
                udi_log("failed to write control trigger to pipe for %a: %e",
                        iter->id,
                        errno);
                udi_abort();
                return -1;
            }

            if ( thr != NULL ) {
                udi_log("thread %a waiting to be released after transferring control to thread %a",
                        thr->id, iter->id);
                unsigned char trigger;
                if ( read(thr->control_read, &trigger, 1) != 1 ) {
                    udi_log("failed to read control trigger from pipe: %a", errno);
                    udi_abort();
                    return -1;
                }

                if ( trigger != sentinel ) {
                    udi_abort();
                    return -1;
                }
            }
        }else{
            // clear "lock" for future entrances to block_other_threads
            //
            // Note: it's possible that the sync var is already 0 when the first thread was created
            // -- just ignore this case as the below code handles this case correctly
            __sync_val_compare_and_swap(&(thread_barrier.sync_var), 1, 0);

            // release the other threads, if they should be running
            iter = get_thread_list();
            while ( iter != NULL ) {
                if ( iter != thr && iter->ts == UDI_TS_RUNNING ) {
                    if ( write(iter->control_write, &sentinel, 1) != 1 ) {
                        udi_log("failed to write control trigger to pipe for %a: %e",
                                iter->id,
                                errno);
                        udi_abort();
                        return -1;
                    }
                }
                iter = iter->next_thread;
            }

            udi_log("thread %a released other threads", get_user_thread_id());

            if ( thr != NULL && thr->ts == UDI_TS_SUSPENDED ) {
                // If the current thread was suspended in this signal handler call, block here
                udi_log("thread %a waiting to be released after releasing threads", thr->id);

                read_sentinel(thr->control_read);
            }
        }
    }

    return 0;
}

/**
 * Allocates and populates a string with the thread directory
 *
 * @param thr the thread
 *
 * @return the string
 */ 
static
char *get_thread_dir(thread *thr) {
    char subdir_name[32];

    udi_formatted_str(subdir_name, 32, "%x", thr->id);

    size_t dir_length = strlen(basedir_name) + DS_LEN + strlen(subdir_name) + 1;
    char *thread_dir = (char *)udi_malloc(dir_length);
    if (thread_dir == NULL) {
        udi_log("malloc failed: %e", errno);
        return NULL;
    }

    udi_formatted_str(thread_dir, dir_length, "%s/%s", basedir_name, subdir_name);
    return thread_dir;
}

/**
 * Allocates and populates a string with the thread directory
 *
 * @param thr the thread
 * @param thread_dir the thread directory
 * @param filename the filename
 *
 * @return the string
 */
static
char *get_thread_file(thread *thr, char *thread_dir, const char *filename) {
    size_t file_length = strlen(thread_dir) + DS_LEN + strlen(filename) + 1;
    char *thread_file = (char *)udi_malloc(file_length);
    if (thread_file == NULL) {
        udi_log("malloc failed: %e", errno);
        return NULL;
    }

    udi_formatted_str(thread_file, file_length, "%s/%s", thread_dir, filename);
    return thread_file;
}

int thread_create_callback(thread *thr, udi_errmsg *errmsg) {

    // create the filesystem elements
    char *thread_dir = NULL, *response_file = NULL, *request_file = NULL;

    int result = 0;
    do {
        thread_dir = get_thread_dir(thr);
        if (thread_dir == NULL) {
            result = -1;
            break;
        }

        if (mkdir(thread_dir, S_IRWXG | S_IRWXU) == -1) {
            udi_set_errmsg(errmsg,
                           "failed to create directory %s: %e",
                           thread_dir,
                           errno);
            result = -1;
            break;
        }

        response_file = get_thread_file(thr, thread_dir, RESPONSE_FILE_NAME);
        if (response_file == NULL) {
            result = -1;
            break;
        }

        request_file = get_thread_file(thr, thread_dir, REQUEST_FILE_NAME);
        if (request_file == NULL) {
            result = -1;
            break;
        }

        if (mkfifo(request_file, S_IRWXG | S_IRWXU) == -1) {
            udi_set_errmsg(errmsg,
                           "failed to create request pipe %s: %e",
                           request_file,
                           errno);
            result = -1;
            break;
        }

        if (mkfifo(response_file, S_IRWXG | S_IRWXU) == -1) {
            udi_set_errmsg(errmsg, "failed to create response pipe %s: %e",
                           response_file,
                           errno);
            result = -1;
            break;
        }
    }while(0);

    if (result != 0) {
        udi_log("%s", errmsg->msg);
    }

    udi_free(thread_dir);
    udi_free(response_file);
    udi_free(request_file);

    return result;
}

struct thr_resp_ctx {
    thread *thr;
    const char *response_file;
};

static
int thread_create_response_fd_callback(void *ctx, udirt_fd *resp_fd, udi_errmsg *errmsg) {
    struct thr_resp_ctx *resp_ctx = (struct thr_resp_ctx *)ctx;

    resp_ctx->thr->response_handle = open(resp_ctx->response_file, O_WRONLY);
    if (resp_ctx->thr->response_handle == -1) {
        udi_set_errmsg(errmsg,
                       "failed to open %s: %e",
                       resp_ctx->response_file,
                       errno);
        return RESULT_ERROR;
    }

    *resp_fd = resp_ctx->thr->response_handle;
    return RESULT_SUCCESS;
}

int thread_create_handshake(thread *thr, udi_errmsg *errmsg) {

    char *thread_dir = NULL, *response_file = NULL, *request_file = NULL;
    int result = 0;

    do {
        thread_dir = get_thread_dir(thr);
        if (thread_dir == NULL) {
            result = RESULT_ERROR;
            break;
        }

        response_file = get_thread_file(thr, thread_dir, RESPONSE_FILE_NAME);
        if (response_file == NULL) {
            result = RESULT_ERROR;
            break;
        }

        request_file = get_thread_file(thr, thread_dir, REQUEST_FILE_NAME);
        if (request_file == NULL) {
            result = RESULT_ERROR;
            break;
        }

        thr->request_handle = open(request_file, O_RDONLY);
        if (thr->request_handle == -1) {
            udi_set_errmsg(errmsg,
                           "failed to open %s: %e",
                           request_file,
                           errno);
            result = RESULT_ERROR;
            break;
        }

        struct thr_resp_ctx resp_ctx;
        resp_ctx.thr = thr;
        resp_ctx.response_file = response_file;

        result = perform_init_handshake(thr->request_handle,
                                        thread_create_response_fd_callback,
                                        &resp_ctx,
                                        thr->id,
                                        errmsg);
        if (result != RESULT_SUCCESS) {
            udi_log("failed to complete init handshake for thread %a", thr->id);
            break;
        }
    }while(0);

    if (result != RESULT_SUCCESS) {
        udi_log("failed to complete handshake for thread: %s", errmsg->msg);
    }

    udi_free(thread_dir);
    udi_free(response_file);
    udi_free(request_file);

    return result;
}

int thread_death_callback(thread *thr, udi_errmsg *errmsg) {
    thr->dead = 1;
    udi_log("thread %s marked dead", thr->id);

    // close the response file
    if ( close(thr->response_handle) != 0 ) {
        udi_set_errmsg(errmsg,
                       "failed to close response handle for thread %a: %s",
                       thr->id,
                       errno);
        udi_log("%s", errmsg->msg);
        return -1;
    }

    return 0;
}

int thread_death_handshake(thread *thr, udi_errmsg *errmsg) {

    if (!thr->dead) {
        udi_set_errmsg(errmsg,
                       "thread %a is not dead, not completing handshake",
                       thr->id);
        udi_log("%s", errmsg->msg);
        return -1;
    }

    // close the request file
    if ( close(thr->request_handle) != 0 ) {
        udi_set_errmsg(errmsg,
                       "failed to close request handle for thread %a: %e",
                       thr->id,
                       errno);
        udi_log("%s", errmsg->msg);
        return -1;
    }

    // delete the pipes and the thread directory
    char *thread_dir = NULL, *response_file = NULL, *request_file = NULL;
    int result = 0;
    do {
        thread_dir = get_thread_dir(thr);
        if (thread_dir == NULL) {
            result = -1;
            break;
        }

        response_file = get_thread_file(thr, thread_dir, RESPONSE_FILE_NAME);
        if (response_file == NULL) {
            result = -1;
            break;
        }

        if ( unlink(response_file) != 0 ) {
            udi_set_errmsg(errmsg,
                           "failed to unlink %s: %e",
                           response_file,
                           errno);
            result = -1;
            break;
        }

        request_file = get_thread_file(thr, thread_dir, REQUEST_FILE_NAME);
        if (request_file == NULL) {
            result = -1;
            break;
        }

        if ( unlink(request_file) != 0 ) {
            udi_set_errmsg(errmsg,
                           "failed to unlink %s: %e",
                           request_file,
                           errno);
            result = -1;
            break;
        }

        if ( rmdir(thread_dir) != 0 ) {
            udi_set_errmsg(errmsg, "failed to rmdir %s: %e",
                           thread_dir,
                           errno);
            result = -1;
            break;
        }
    }while(0);

    if ( result != 0 ) {
        udi_log("%s", errmsg->msg);
    }

    udi_free(thread_dir);
    udi_free(response_file);
    udi_free(request_file);

    destroy_thread(thr);

    return 0;
}

static
int process_response_fd_callback(void *ctx, udirt_fd *resp_fd, udi_errmsg *errmsg) {
    thread **output = (thread **)ctx;

    response_handle = open(responsefile_name, O_WRONLY);
    if (response_handle == -1) {
        udi_set_errmsg(errmsg,
                       "error opening response file fifo: %e",
                       errno);
        return RESULT_ERROR;
    }

    thread *thr = create_initial_thread();
    if (thr == NULL) {
        udi_set_errmsg(errmsg,
                       "failed to create initial thread");
        return RESULT_ERROR;
    }

    if ( thread_create_callback(thr, errmsg) != 0 ) {
        udi_log("failed to create thread-specific files: %s", errmsg->msg);
        return RESULT_ERROR;
    }

    events_handle = open(eventsfile_name, O_WRONLY);
    if (events_handle == -1) {
        udi_set_errmsg(errmsg,
                       "error opening response file fifo: %e",
                       errno);
        return RESULT_ERROR;
    }

    *output = thr;
    *resp_fd = response_handle;
    return RESULT_SUCCESS;
}

/**
 * Performs the initiation handshake with the debugger.
 *
 * @param errmsg the error message populated on error
 *
 * @return RESULT_SUCCESS on success
 */
static
int handshake_with_debugger(udi_errmsg *errmsg) {
    int result = RESULT_SUCCESS;
    do {
        if ((request_handle = open(requestfile_name, O_RDONLY)) == -1 ) {
            udi_set_errmsg(errmsg,
                           "error opening request file fifo: %e",
                           errno);
            result = RESULT_ERROR;
            break;
        }

        thread *thr = NULL;
        result = perform_init_handshake(request_handle,
                                        process_response_fd_callback,
                                        &thr,
                                        get_user_thread_id(),
                                        errmsg);
        if (result != RESULT_SUCCESS) {
            break;
        }

        result = thread_create_handshake(thr, errmsg);
        if (result != RESULT_SUCCESS) {
            udi_log("failed to complete thread create handshake: %s", errmsg->msg);
            break;
        }


        // after sending init response, any debugging operations are valid
        udi_enabled = 1;

    }while(0);

    return result;
}

void reinit_udirt() {
    // TODO
}

/** The entry point for initialization declaration */
void init_udirt() UDI_CONSTRUCTOR;

/**
 * The entry point for initialization
 */
void init_udirt() {
    udi_errmsg errmsg;
    errmsg.size = ERRMSG_SIZE;
    errmsg.msg[ERRMSG_SIZE-1] = '\0';

    enable_debug_logging();

    if ( testing_udirt != NULL ) return;

    global_initialization();

    thread *request_thr = NULL;
    sigset_t original_set;
    int result;
    do {
        sigset_t block_set;
        sigfillset(&block_set);

        // Block any signals during initialization
        if ( setsigmask(SIG_SETMASK, &block_set, &original_set) == -1 ) {
            udi_set_errmsg(&errmsg,
                           "failed to block all signals: %e",
                           errno);
            result = RESULT_ERROR;
            break;
        }

        if ( locate_wrapper_functions(&errmsg) != 0 ) {
            result = RESULT_ERROR;
            break;
        }

        if ( locate_thread_wrapper_functions(&errmsg) != 0) {
            result = RESULT_ERROR;
            break;
        }

        if ( initialize_thread_sync() != 0 ) {
            udi_set_errmsg(&errmsg,
                           "failed to initialize thread sync");
            result = RESULT_ERROR;
            break;
        }

        int signal_result = setup_signal_handlers();
        if ( signal_result != 0 ) {
            udi_set_errmsg(&errmsg,
                           "failed to setup signal handlers: %e",
                           signal_result);
            result = RESULT_ERROR;
            break;
        }

        if ( install_event_breakpoints(&errmsg) != 0 ) {
            result = RESULT_ERROR;
            break;
        }

        if ( create_udi_filesystem() != 0 ) {
            udi_set_errmsg(&errmsg,
                           "failed to create udi filesystem");
            result = RESULT_ERROR;
            break;
        }

        result = handshake_with_debugger(&errmsg);
        if (result != RESULT_SUCCESS) {
            udi_log("failed to complete handshake with debugger");
            break;
        }

        result = wait_and_execute_command(&errmsg, &request_thr);
        if ( result != RESULT_SUCCESS ) {
            udi_log("failed to handle initial command");
            break;
        }
    } while(0);

    if (result != RESULT_SUCCESS ) {
        udi_enabled = 0;

        udi_log("initialization failed: %s", errmsg.msg);
        udi_abort();
    }else{
        udi_log("initialization completed");
    }

    // Explicitly ignore errors
    setsigmask(SIG_SETMASK, &original_set, NULL);
}

int read_from(udirt_fd fd, uint8_t *dst, size_t length)
{
    size_t total = 0;
    while (total < length) {
        ssize_t num_read = read(fd,
                                dst + total,
                                length - total);

        if ( num_read == 0 ) {
            // Treat end-of-file as a separate error
            return -1;
        }

        if (num_read < 0) {
            if (errno == EINTR) continue;
            return errno;
        }
        total += num_read;
    }

    return 0;
}

/**
 * Performs handling necessary to handle a pipe write failure gracefully.
 */
static
void handle_pipe_write_failure() {
    pipe_write_failure = 1;

    sigset_t set;

    if ( sigpending(&set) != 0 ) {
        udi_log("failed to get pending signals: %e", errno);
        return;
    }

    if ( sigismember(&set, SIGPIPE) != 1 ) {
        udi_log("SIGPIPE is not pending, cannot handle write failure");
        return;
    }

    sigset_t cur_set;
    if ( setsigmask(SIG_BLOCK, NULL, &cur_set) != 0 ) {
        udi_log("failed to get current signals: %e", errno);
        return;
    }
    sigdelset(&set, SIGPIPE);

    sigsuspend(&set);
    if ( errno != EINTR ) {
        udi_log("failed to wait for signal to be delivered: %e",
                errno);
        return;
    }

    pipe_write_failure = 0;
}

int write_to(udirt_fd fd, const uint8_t *src, size_t length) {

    int errnum = 0;
    size_t total = 0;
    while (total < length) {
        ssize_t num_written = write(fd,
                                    ((unsigned char *)src) + total,
                                    length - total);
        if ( num_written < 0 ) {
            if ( errno == EINTR ) continue;
            errnum = errno;
            break;
        }

        total += num_written;
    }

    if (errnum == EPIPE) {
        handle_pipe_write_failure();
    }

    return errnum;
}

udirt_fd udi_log_fd() {
    return STDERR_FILENO;
}

void udi_log_error(format_cb cb, void *ctx, int error) {
    char buf[64];
    memset(buf, 0, 64);

    strerror_r(error, buf, 64);

    udi_log_string(cb, ctx, buf);
}

