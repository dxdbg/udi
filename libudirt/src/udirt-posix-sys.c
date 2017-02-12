/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// OS related handling (syscalls and signals)

// This header needs to be included first because it sets feature macros
#include "udirt-platform.h"

// This nonsense is needed because Linux headers redefine signal as __sysv_signal
void (*signal(int signum, void (*handler)(int)) )(int) __asm__ ("" "signal");

#include "udirt-posix.h"

#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>

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
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg->msg, errmsg_tmp, errmsg->size-1);
            errnum = -1;
            break;
        }

        real_fork = (fork_type) dlsym(UDI_RTLD_NEXT, "fork");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg->msg, errmsg_tmp, errmsg->size-1);
            errnum = -1;
            break;
        }

        real_execve = (execve_type) dlsym(UDI_RTLD_NEXT, "execve");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
            strncpy(errmsg->msg, errmsg_tmp, errmsg->size-1);
            errnum = -1;
            break;
        }

        real_signal = (signal_type) dlsym(UDI_RTLD_NEXT, "signal");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_printf("symbol lookup error: %s\n", errmsg_tmp);
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
            udi_printf("%s\n", "failed to create exit breakpoint");
            errnum = -1;
            break;
        }

        errnum = install_breakpoint(exit_bp, errmsg);

        if ( errnum != 0 ) {
            udi_printf("%s\n", "failed to install exit breakpoint");
            errnum = -1;
            break;
        }

        if (get_multithread_capable()) {
            errnum = install_thread_event_breakpoints(errmsg);
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

    udi_printf("exit entered with status %d\n", status);
    exiting = 1;
    result = handle_exit_event(get_user_thread_id(), status, errmsg);
    if ( result != RESULT_SUCCESS ) {
        udi_printf("failed to report exit status of %d\n", status);
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

        udi_printf(">>> fork entry for 0x%"PRIx64"/%u\n",
                   get_user_thread_id(),
                   get_kernel_thread_id());

        // Need to continue waiting until this thread owns the control of the process
        int block_result = 1;
        while (block_result > 0) {
            block_result = block_other_threads();
            if ( block_result == -1 ) {
                udi_printf("%s\n", "failed to block other threads");
                errno = EAGAIN;
                return -1;
            }
        }

        udi_errmsg errmsg;
        errmsg.size = ERRMSG_SIZE;
        errmsg.msg[ERRMSG_SIZE-1] = '\0';

        int result = handle_fork_event(thread_id, child, &errmsg);
        if (result != RESULT_SUCCESS) {
            udi_printf("failed to report fork event: %s\n", errmsg.msg);
            errno = EAGAIN;
            return -1;
        }

        thread *thr = NULL;
        result = wait_and_execute_command(&errmsg, &thr);
        if (result == RESULT_ERROR) {
            udi_printf("failed to execute command after fork: %s\n", errmsg.msg);
            errno = EAGAIN;
            return -1;
        }

        int release_result = release_other_threads();
        if ( release_result != 0 ) {
            udi_printf("%s\n", "failed to release other threads");
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
        int signal_index = signal_map[(signum % MAX_SIGNAL_NUM)];
        if ( oldact != NULL ) {
            *oldact = app_actions[signal_index];
        }

        if ( act != NULL ) {
            app_actions[signal_index] = *act;
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

    return is_thread_event_breakpoint(bp);
}

int handle_event_breakpoint(breakpoint *bp, const void *in_context, udi_errmsg *errmsg) {

    const ucontext_t *context = (const ucontext_t *)in_context;

    if (bp == exit_bp) {
        return handle_exit_breakpoint(context, errmsg);
    }


    return handle_thread_event_breakpoint(bp, context, errmsg);
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

    // Sanity check
    if ( (sizeof(signals) / sizeof(int)) != NUM_SIGNALS ) {
        udi_printf("%s\n", "ASSERT FAIL: signals array length != NUM_SIGNALS");
        return -1;
    }

    for(i = 0; i < NUM_SIGNALS; ++i) {
        if ( real_sigaction(signals[i], &default_lib_action, &app_actions[i]) != 0 ) {
            errnum = errno;
            break;
        }
    }

    return errnum;
}
