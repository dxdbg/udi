/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// OS related handling (syscalls, signals, library loading)

// This header needs to be included first because it sets feature macros
#include "udirt-platform.h"

// This nonsense is needed because Linux headers redefine signal as __sysv_signal
void (*signal(int signum, void (*handler)(int)) )(int) __asm__ ("" "signal");

#include "udirt-posix.h"

#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>

// exported constants //

// library wrapping
void *UDI_RTLD_NEXT = RTLD_NEXT;

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
#if defined(SIGSTKFLT)
    SIGSTKFLT,
#else
    0,
#endif
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
#if defined(SIGPWR)
    SIGPWR,
#else
    0,
#endif
    SIGSYS
};

// This is the number of elements in the signals array
#define NUM_SIGNALS 29

struct app_sigaction {
    int signal;
    struct sigaction action;
};

static struct app_sigaction app_actions[NUM_SIGNALS];
static struct sigaction default_lib_action;

int exiting = 0;

// wrapper function pointers
sigaction_type real_sigaction;
fork_type real_fork;
execve_type real_execve;
signal_type real_signal;

// event breakpoints
static breakpoint *exit_bp = NULL;

/**
 * Use dynamic loader to locate functions that are wrapped by library
 * wrappers
 *
 * @param errmsg the errmsg populated on error
 *
 * @return 0 on success; non-zero on failure
 */
int locate_wrapper_functions(udi_errmsg *errmsg) {
    int errnum = 0;
    char *errmsg_tmp;

    // reset dlerror()
    dlerror();

    do {
        // locate symbols for wrapper functions
        real_sigaction = (sigaction_type) dlsym(UDI_RTLD_NEXT, "sigaction");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_log("symbol lookup error: %s", errmsg_tmp);
            strncpy(errmsg->msg, errmsg_tmp, errmsg->size-1);
            errnum = -1;
            break;
        }

        real_fork = (fork_type) dlsym(UDI_RTLD_NEXT, "fork");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_log("symbol lookup error: %s", errmsg_tmp);
            strncpy(errmsg->msg, errmsg_tmp, errmsg->size-1);
            errnum = -1;
            break;
        }

        real_execve = (execve_type) dlsym(UDI_RTLD_NEXT, "execve");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_log("symbol lookup error: %s", errmsg_tmp);
            strncpy(errmsg->msg, errmsg_tmp, errmsg->size-1);
            errnum = -1;
            break;
        }

        real_signal = (signal_type) dlsym(UDI_RTLD_NEXT, "signal");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_log("symbol lookup error: %s", errmsg_tmp);
            strncpy(errmsg->msg, errmsg_tmp, errmsg->size-1);
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
 *
 * @return 0 on success; non-zero on failure
 */
int install_event_breakpoints(udi_errmsg *errmsg) {
    int errnum = 0;

    do {
        // Exit cannot be wrapped because Linux executables can call it
        // directly and do not pass through the PLT
        exit_bp = create_breakpoint((uint64_t)exit);
        if ( exit_bp == NULL ) {
            udi_log("failed to create exit breakpoint");
            errnum = -1;
            break;
        }

        errnum = install_breakpoint(exit_bp, errmsg);

        if ( errnum != 0 ) {
            udi_log("failed to install exit breakpoint");
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
 *
 * @return the result
 */
static
int handle_exit_breakpoint(const ucontext_t *context, udi_errmsg *errmsg) {

    int status = 0;
    int result = get_exit_argument(context, &status, errmsg);
    if ( result != RESULT_SUCCESS ) {
        return result;
    }

    udi_log("exit entered with status %d", status);
    exiting = 1;
    result = handle_exit_event(get_user_thread_id(), status, errmsg);
    if ( result != RESULT_SUCCESS ) {
        udi_log("failed to report exit status of %d", status);
    }
    return result;
}

/**
 * The wrapper function for the fork system call.
 *
 * See fork manpage for description.
 */
pid_t fork() {

    pid_t child = real_fork();
    if ( child == 0 ) {
        reinit_udirt();
    }else{
        uint64_t thread_id = get_user_thread_id();

        udi_log(">>> fork entry for %a/%a",
                get_user_thread_id(),
                get_kernel_thread_id());

        // Need to continue waiting until this thread owns the control of the process
        int block_result = 1;
        while (block_result > 0) {
            block_result = block_other_threads();
            if ( block_result == -1 ) {
                udi_log("failed to block other threads");
                errno = EAGAIN;
                return -1;
            }
        }

        udi_errmsg errmsg;
        errmsg.size = ERRMSG_SIZE;
        errmsg.msg[ERRMSG_SIZE-1] = '\0';

        int result = handle_fork_event(thread_id, child, &errmsg);
        if (result != RESULT_SUCCESS) {
            udi_log("failed to report fork event: %s", errmsg.msg);
            errno = EAGAIN;
            return -1;
        }

        thread *thr = NULL;
        result = wait_and_execute_command(&errmsg, &thr);
        if (result == RESULT_ERROR) {
            udi_log("failed to execute command after fork: %s", errmsg.msg);
            errno = EAGAIN;
            return -1;
        }

        int release_result = release_other_threads();
        if ( release_result != 0 ) {
            udi_log("failed to release other threads");
            errno = EAGAIN;
            return -1;
        }
    }

    return child;
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
    udi_log("wrapped call to sigaction for signal %d", signum);

    // Block signals while doing this to avoid a race where a signal is delivered
    // while validating the arguments
    sigset_t full_set, orig_set;
    sigfillset(&full_set);

    int block_result = setsigmask(SIG_SETMASK, &full_set, &orig_set);
    if ( block_result != 0 ) return EINVAL;

    int result = 0;
    do {
        // Validate the arguments
        int result = real_sigaction(signum, act, oldact);
        if ( result != 0 ) break;

        // Reset action back to library default
        result = real_sigaction(signum, &default_lib_action, NULL);
        if ( result != 0 ) break;

        // Store new application action for future use

        int found = 0;
        int i;
        for (i = 0; i < NUM_SIGNALS; ++i) {
            if (app_actions[i].signal == signum) {
                found = 1;
                if ( oldact != NULL ) {
                    *oldact = app_actions[i].action;
                }

                if ( act != NULL ) {
                    app_actions[i].action = *act;
                }
            }
        }

        if ( !found ) {
            udi_log("failed to intercept call to sigaction for %d", signum);
            return EINVAL;
        }
    }while(0);

    // Unblock signals
    block_result = setsigmask(SIG_SETMASK, &orig_set, NULL);
    if ( block_result != 0 ) return EINVAL;

    return result;
}

/**
 * Wrapper function for signal
 *
 * See the manpage for signal for information about the parameters and return
 */
void (*signal(int signum, void (*handler)(int)) )(int) {

    struct sigaction current, old;
    current.sa_handler = handler;

    if ( sigaction(signum, &current, &old) != 0 ) {
        return SIG_ERR;
    }

    return old.sa_handler;
}

int is_event_breakpoint(breakpoint *bp) {
    if (bp == exit_bp) {
        return 1;
    }

    return 0;
}

int handle_event_breakpoint(breakpoint *bp, const void *in_context, udi_errmsg *errmsg) {

    const ucontext_t *context = (const ucontext_t *)in_context;

    if (bp == exit_bp) {
        return handle_exit_breakpoint(context, errmsg);
    }

    return RESULT_ERROR;
}


/**
 * The entry point for passing a signal to a user signal handler
 *
 * See manpage for SA_SIGINFO function.
 */
void app_signal_handler(int signum, siginfo_t *siginfo, void *v_context) {

    struct app_sigaction *app_action = NULL;
    for (int i = 0; i < NUM_SIGNALS; ++i) {
        if (app_actions[i].signal == signum) {
            app_action = &app_actions[i];
        }
    }

    if (app_action == NULL) {
        udi_log("Signal %d does not have an app action", signum);
        return;
    }

    if ( (void *)app_action->action.sa_sigaction == (void *)SIG_IGN ) {
        udi_log("Signal %d ignored, not passing to application", signum);
        return;
    }

    if ( (void *)app_actions->action.sa_sigaction == (void *)SIG_DFL ) {
        // TODO need to emulate the default action
        return;
    }

    sigset_t cur_set;
    if ( setsigmask(SIG_SETMASK, &app_actions->action.sa_mask, &cur_set) != 0 )
    {
        udi_log("failed to adjust blocked signals for application handler: %e", errno);
    }

    app_actions->action.sa_sigaction(signum, siginfo, v_context);

    if ( setsigmask(SIG_SETMASK, &cur_set, NULL) != 0 ) {
        udi_log("failed to reset blocked signals after application handler: %e", errno);
    }
}

/**
 * Sets up the signal handlers for all catchable signals
 *
 * @return 0 on success; non-zero on failure
 */
int setup_signal_handlers() {

    // Sanity check
    if ( (sizeof(signals) / sizeof(int)) != NUM_SIGNALS ) {
        udi_log("ASSERT FAIL: signals array length != NUM_SIGNALS");
        return -1;
    }

    // Define the default sigaction for the library
    memset(&default_lib_action, 0, sizeof(struct sigaction));
    default_lib_action.sa_sigaction = signal_entry_point;
    sigfillset(&(default_lib_action.sa_mask));
    default_lib_action.sa_flags = SA_SIGINFO | SA_NODEFER;

    // initialize application sigactions
    int i;
    for (i = 0; i < NUM_SIGNALS; ++i) {
        memset(&app_actions[i], 0, sizeof(struct app_sigaction));
        app_actions[i].signal = signals[i];
        app_actions[i].action.sa_handler = SIG_DFL;
    }

    int errnum = 0;
    for(i = 0; i < NUM_SIGNALS; ++i) {
        int signum = signals[i];
        if (signum != 0) {
            if ( real_sigaction(signum, &default_lib_action, &(app_actions[i].action)) != 0 ) {
                udi_log("failed to register handler for %d", signals[i]);
                errnum = errno;
                break;
            }
        }
    }

    return errnum;
}

int uninstall_signal_handlers() {
    int i;
    for(i = 0; i < NUM_SIGNALS; ++i) {
        if ( signals[i] == 0 ) continue;

        if ( signals[i] == SIGPIPE && pipe_write_failure ) continue;

        if ( real_sigaction(signals[i], &(app_actions[i].action), NULL) != 0 ) {
            udi_log("failed to reset signal handler for %d: %e",
                    signals[i],
                    errno);
            return errno;
        }
    }

    return 0;
}
