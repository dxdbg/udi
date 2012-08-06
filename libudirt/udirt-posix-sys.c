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

// OS related handling (syscalls and signals)

// This header needs to be included first because it sets feature macros
#include "udirt-platform.h"

#include "udirt-posix.h"

#include <errno.h>

// Signal handling
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

struct sigaction app_actions[NUM_SIGNALS];
int signal_map[MAX_SIGNAL_NUM]; // Used to map signals to their application action
struct sigaction default_lib_action;

// wrapper function pointers
sigaction_type real_sigaction;
fork_type real_fork;
execve_type real_execve;

// event breakpoints
breakpoint *exit_bp = NULL;

/* to be used by system call library function wrappers
static
void wait_and_execute_command_with_response() {
    char errmsg[ERRMSG_SIZE];
    errmsg[ERRMSG_SIZE-1] = '\0';

    int req_result = wait_and_execute_command(errmsg, ERRMSG_SIZE);

    if ( req_result != REQ_SUCCESS ) {
        if ( req_result > REQ_SUCCESS ) {
            strncpy(errmsg, strerror(req_result), ERRMSG_SIZE-1);
        }

        udi_printf("failed to process command: %s\n", errmsg);

        udi_response resp = create_response_error(errmsg, ERRMSG_SIZE);
        if ( resp.packed_data != NULL ) {
            // explicitly ignore errors
            write_response(&resp);
            udi_free(resp.packed_data);
        }
    }
}
*/

/**
 * Use dynamic loader to locate functions that are wrapped by library
 * wrappers
 *
 * @param errmsg the errmsg populated on error
 * @param errmsg_size the size of the errmsg
 *
 * @return 0 on success; non-zero on failure
 */
int locate_wrapper_functions(char *errmsg, unsigned int errmsg_size) {
    int errnum = 0;
    char *errmsg_tmp;

    // reset dlerror()
    dlerror();

    do {
        // locate symbols for wrapper functions
        real_sigaction = (sigaction_type) dlsym(UDI_RTLD_NEXT, "sigaction");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg, errmsg_tmp, errmsg_size-1);
            errnum = -1;
            break;
        }

        real_fork = (fork_type) dlsym(UDI_RTLD_NEXT, "fork");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg, errmsg_tmp, errmsg_size-1);
            errnum = -1;
            break;
        }

        real_execve = (execve_type) dlsym(UDI_RTLD_NEXT, "execve");
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

/**
 * Install any breakpoints necessary to capture any interesting events
 * that cannot be captured by wrapping functions
 *
 * @param errmsg the errmsg populated on error
 * @param errmsg_size the size of the errmsg on error
 *
 * @return 0 on success; non-zero on failure
 */
int install_event_breakpoints(char *errmsg, unsigned int errmsg_size) {
    int errnum = 0;
    
    do {
        int exit_inst_length = get_exit_inst_length(exit, errmsg, errmsg_size);
        if ( exit_inst_length <= 0 ) {
            udi_printf("%s\n", "failed to determine length of instruction for exit breakpoint");
            errnum = -1;
            break;
        }

        // Exit cannot be wrappped because Linux executables can call it
        // directly and do not pass through the PLT
        exit_bp = create_breakpoint((udi_address)(unsigned long)exit,
                exit_inst_length);
        if ( exit_bp == NULL ) {
            udi_printf("%s\n", "failed to create exit breakpoint");
            errnum = -1;
            break;
        }

        errnum = install_breakpoint(exit_bp, errmsg, errmsg_size);

        if ( errnum != 0 ) {
            udi_printf("%s\n", "failed to install exit breakpoint");
            errnum = -1;
            break;
        }
    }while(0);

    return errnum;
}

/**
 * Handle the occurence of a hit on the exit breakpoint. This includes gathering all information
 * neccessary to create the exit event.
 *
 * @param context the context for the breakpoint
 * @param errmsg the error message populated on error
 * @param errmsg_size the size of the error message
 *
 * @return the information extracted about the exit event
 */
event_result handle_exit_breakpoint(const ucontext_t *context, char *errmsg, unsigned int errmsg_size) {

    event_result result;
    result.failure = 0;
    result.wait_for_request = 1;

    exit_result exit_result = get_exit_argument(context, errmsg, errmsg_size);

    if ( exit_result.failure ) {
        result.failure = exit_result.failure;
        return result;
    }

    udi_printf("exit entered with status %d\n", exit_result.status);

    // create the event
    udi_event_internal exit_event = create_event_exit(exit_result.status);

    // Explicitly ignore errors as there is no way to report them
    do {
        if ( exit_event.packed_data == NULL ) {
            result.failure = 1;
            break;
        }

        result.failure = write_event(&exit_event);

        udi_free(exit_event.packed_data);
    }while(0);

    if ( result.failure ) {
        udi_printf("failed to report exit status of %d\n", exit_result.status);
    }

    return result;
}

/**
 * The wrapper function for the fork system call. This is TODO
 */
pid_t fork()
{
    return real_fork();
}

/**
 * The wrapper function for the execve system call. This is TODO.
 *
 * See execve manpage for description of parameters.
 */
int execve(const char *filename, char *const argv[],
        char *const envp[])
{
    // TODO wrapper function stuff

    return real_execve(filename, argv, envp);
}

/**
 * Wrapper function for sigaction.
 *
 * See sigaction manpage for description of parameters.
 *
 * Registers the passed sigaction with user sigactions maintained
 * by the library.
 */
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

/**
 * The entry point for passing a signal to a user signal handler
 *
 * See manpage for SA_SIGINFO function.
 */
void app_signal_handler(int signal, siginfo_t *siginfo, void *v_context) {
    int signal_index = signal_map[(signal % MAX_SIGNAL_NUM)];

    if ( (void *)app_actions[signal_index].sa_sigaction == (void *)SIG_IGN ) {
        udi_printf("Signal %d ignored, not passing to application\n", signal);
        return;
    }

    if ( (void *)app_actions[signal_index].sa_sigaction == (void *)SIG_DFL ) {
        // TODO need to emulate the default action
        return;
    }

    sigset_t cur_set;

    if ( setsigmask(SIG_SETMASK, &app_actions[signal_index].sa_mask, 
            &cur_set) != 0 )
    {
        udi_printf("failed to adjust blocked signals for application handler: %s\n",
                strerror(errno));
    }

    app_actions[signal_index].sa_sigaction(signal, siginfo, v_context);

    if ( setsigmask(SIG_SETMASK, &cur_set, NULL) != 0 ) {
        udi_printf("failed to reset blocked signals after application handler: %s\n",
                strerror(errno));
    }
}

/**
 * Sets up the signal handlers for all catchable signals
 *
 * @return 0 on success; non-zero on failure
 */
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
