/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Thread support for pthreads

// This needs to be included first because it sets feature macros
#include "udirt-platform.h"

#include <inttypes.h>
#include <unistd.h>
#include <errno.h>

#include "udi.h"
#include "udirt.h"
#include "udirt-posix.h"


// pthread function types and definitions

// these need to be weak to avoid linking pthreads with a target that doesn't link pthreads
extern int pthread_sigmask(int how, const sigset_t *new_set, sigset_t *old_set) UDI_WEAK;
extern pthread_t pthread_self() UDI_WEAK;

// the threads list
static int num_threads = 0;
static thread *threads = NULL;

int THREAD_SUSPEND_SIGNAL = SIGSYS;

inline
int get_multithread_capable() {
    return pthread_sigmask != 0;
}

int get_multithreaded() {
    return get_multithread_capable() && get_num_threads() > 1;
}

/**
 * Sets the signal mask for all threads in this process
 *
 * @return 0, on success; non-zero on failure
 */
int setsigmask(int how, const sigset_t *new_set, sigset_t *old_set) {
    // Only use pthread_sigmask when it is available
    if ( pthread_sigmask ) {
        return pthread_sigmask(how, new_set, old_set);
    }

    return sigprocmask(how, new_set, old_set);
}

/**
 * @return the user thread id for the currently executing thread
 */
uint64_t get_user_thread_id() {

    // pthread_self tends to be linked in single thread processes as well
    if (pthread_self &&  get_multithread_capable() ) {
        return (uint64_t)pthread_self();
    }

    return (uint64_t)UDI_SINGLE_THREAD_ID;
}

typedef int (*pthread_create_type)(pthread_t *,
                                   const pthread_attr_t *,
                                   void *(*)(void *),
                                   void *);
pthread_create_type real_pthread_create;

typedef void (*pthread_exit_type)(void *) __attribute__((noreturn));
pthread_exit_type real_pthread_exit;

int locate_thread_wrapper_functions(udi_errmsg *errmsg) {
    int errnum = 0;
    char *errmsg_tmp;

    // reset dlerror()
    dlerror();

    do {
        real_pthread_create = (pthread_create_type) dlsym(UDI_RTLD_NEXT, "pthread_create");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_log("symbol lookup error: %s", errmsg_tmp);
            strncpy(errmsg->msg, errmsg_tmp, errmsg->size-1);
            errnum = -1;
            break;
        }
        udi_log("real pthread create at %a", (uint64_t)real_pthread_create);

        real_pthread_exit = (pthread_exit_type) dlsym(UDI_RTLD_NEXT, "pthread_exit");
        errmsg_tmp = dlerror();
        if (errmsg_tmp != NULL) {
            udi_log("symbol lookup error: %s", errmsg_tmp);
            strncpy(errmsg->msg, errmsg_tmp, errmsg->size-1);
            errnum = -1;
            break;
        }

        udi_log("real pthread exit at %a", (uint64_t)real_pthread_exit);
    }while(0);

    return errnum;
}

static
int init_control_pipe(int *control_pipe) {

    if ( get_multithread_capable() ) {
        if ( pipe(control_pipe) != 0 ) {
            udi_log("failed to create thread control pipe: %e", errno);
            return -1;
        }
    } else {
        control_pipe[0] = -1;
        control_pipe[1] = -1;
    }

    return 0;
}

static
thread *create_thread_struct(uint64_t tid) {
    int control_pipe[2];

    thread *new_thr = (thread *)udi_malloc(sizeof(thread));
    if (new_thr == NULL) {
        return NULL;
    }
    memset(new_thr, 0, sizeof(thread));

    if ( init_control_pipe(control_pipe) != 0 ) {
        return NULL;
    }

    if ( allocate_context_data(&(new_thr->event_state.context_data)) != 0 ) {
        return NULL;
    }

    new_thr->id = tid;
    new_thr->dead = 0;
    new_thr->request_handle = -1;
    new_thr->response_handle = -1;
    new_thr->next_thread = NULL;
    new_thr->control_read = control_pipe[0];
    new_thr->control_write = control_pipe[1];
    new_thr->ts = UDI_TS_RUNNING;
    new_thr->suspend_pending = 0;
    new_thr->control_thread = 0;
    new_thr->stack_event_pending = 0;

    return new_thr;
}

static
void link_thread(thread *new_thr) {

    // find the end of the list
    thread *cur_thread = threads;
    thread *last_thread = threads;

    while (cur_thread != NULL) {
        last_thread = cur_thread;
        cur_thread = cur_thread->next_thread;
    }

    num_threads++;

    if (last_thread == NULL) {
        threads = new_thr;
    }else{
        last_thread->next_thread = new_thr;
    }

    // global data updated, issue full memory barrier
    __sync_synchronize();
}

/**
 * Creates a thread structure for the specified thread id
 *
 * @param tid the tid
 *
 * @return the created thread structure or NULL if it could not be created
 */
static
thread *create_thread(uint64_t tid) {

    thread *new_thr = create_thread_struct(tid);
    if (new_thr == NULL) {
        return NULL;
    }

    link_thread(new_thr);

    return new_thr;
}

/**
 * @param tid the thread id
 *
 * @return the thread or NULL if none could be found
 */
static
thread *find_thread(uint64_t tid) {
    thread *thr = threads;
    while (thr != NULL) {
        if (thr->id == tid) break;
        thr = thr->next_thread;
    }

    return thr;
}

thread *get_thread_list() {
    return threads;
}

thread *get_next_thread(thread *thr) {
    return thr->next_thread;
}

int is_thread_dead(thread *thr) {
    return thr->dead;
}

static
int handle_thread_create(uint64_t creator_thr)
{
    int result;
    uint64_t tid;

    udi_errmsg errmsg;
    errmsg.size = ERRMSG_SIZE;
    errmsg.msg[ERRMSG_SIZE-1] = '\0';

    do {
        tid = get_user_thread_id();
        udi_log("thread create event for %a", tid);

        thread *thr = create_thread(tid);
        if (thr == NULL) {
            result = RESULT_ERROR;
            break;
        }

        if ( thread_create_callback(thr, &errmsg) != 0 ) {
            result = RESULT_ERROR;
            break;
        }

        result = handle_thread_create_event(creator_thr, tid, &errmsg);
        if (result != RESULT_SUCCESS) {
            break;
        }

        result = thread_create_handshake(thr, &errmsg);
        if (result != RESULT_SUCCESS) {
            break;
        }

        if ( write(thread_barrier.write_handle, &sentinel, 1) != 1 ) {
            udi_log("failed to write trigger to pipe: %e", errno);
            result = RESULT_ERROR;
            break;
        }

        unsigned char trigger = 0;
        while (trigger == 0) {
            int trigger_read_result = read(thr->control_read, &trigger, 1);
            if (trigger_read_result == -1) {
                if (errno == EINTR) {
                    continue;
                }
                udi_log("failed to read control trigger from pipe: %e", errno);
                result = RESULT_ERROR;
                break;
            }
            result = RESULT_SUCCESS;
        }
        if (result != RESULT_SUCCESS) {
            break;
        }

        if ( trigger != sentinel ) {
            udi_abort();
        }
    }while(0);

    if ( result != RESULT_SUCCESS ) {
        udi_log("failed to report thread create of %a: %s",
                tid,
                errmsg.msg);
    }

    return result;
}

static
int handle_thread_death() {

    int result;
    uint64_t tid;

    udi_errmsg errmsg;
    errmsg.size = ERRMSG_SIZE;
    errmsg.msg[ERRMSG_SIZE-1] = '\0';

    do {
        tid = get_user_thread_id();

        udi_log("thread death event for %a", tid);

        thread *thr = find_thread(tid);
        if (thr == NULL) {
            result = RESULT_ERROR;
            break;
        }

        if ( thread_death_callback(thr, &errmsg) != 0 ) {
            result = RESULT_ERROR;
            break;
        }

        result = handle_thread_death_event(tid, &errmsg);
    }while(0);

    if ( result != RESULT_SUCCESS ) {
        udi_log("failed to report thread death of %a: %s",
                tid,
                errmsg.msg);
    }

    return result;
}

static
void report_thread_death() {

    // block signals while reporting the thread death
    sigset_t block_set;
    sigset_t original_set;
    sigfillset(&block_set);

    if (setsigmask(SIG_SETMASK, &block_set, &original_set) == -1) {
        udi_abort();
    }

    udi_log("thread %a dying", get_user_thread_id());

    thread *thr = get_current_thread();
    thr->stack_event_pending = 1;

    int block_result = block_other_threads();
    if (block_result < 0) {
        udi_log("failed to block other threads");
        udi_abort();
        return;
    }

    if (block_result > 0) {
        // the thread should always eventually be the control thread
        udi_abort();
    } else {
        thr->stack_event_pending = 0;

        int death_result = handle_thread_death();
        if (death_result != RESULT_SUCCESS) {
            udi_log("failed to handle thread death");
            udi_abort();
            return;
        }

        udi_errmsg errmsg;
        errmsg.size = ERRMSG_SIZE;
        errmsg.msg[ERRMSG_SIZE-1] = '\0';
        thread *request_thr = NULL;

        int request_result = wait_and_execute_command(&errmsg, &request_thr);
        if (request_result == RESULT_ERROR) {
            udi_log("failed to handle command after thread death");
            udi_abort();
        }

        release_other_threads();
    }

    setsigmask(SIG_SETMASK, &original_set, NULL);
}

void pthread_exit(void *value_ptr)
{
    report_thread_death();

    real_pthread_exit(value_ptr);
}

struct wrapped_thread_context {
    void *(*start_routine)(void *);
    void *arg;
    uint64_t creator_tid;
};

static
void *wrapped_start_routine(void *arg)
{
    struct wrapped_thread_context *context = (struct wrapped_thread_context *)arg;

    void *ret_value = NULL;
    do {
        int create_result = handle_thread_create(context->creator_tid);

        if (create_result != RESULT_SUCCESS) {
            udi_log("failed to handle thread creation");
            udi_abort();
            break;
        }
        ret_value = context->start_routine(context->arg);

        report_thread_death();
    }while(0);

    udi_free(context);

    return ret_value;
}

static
int handshake_with_thread()
{
    udi_errmsg errmsg;
    errmsg.size = ERRMSG_SIZE;
    errmsg.msg[ERRMSG_SIZE-1] = '\0';

    unsigned char trigger = 0;
    if ( read(thread_barrier.read_handle, &trigger, 1) != 1 ) {
        udi_log("failed to read trigger from pipe: %e", errno);
        return RESULT_ERROR;
    }

    if ( trigger != sentinel ) {
        udi_abort();
        return RESULT_ERROR;
    }

    thread *request_thr = NULL;
    int result = wait_and_execute_command(&errmsg, &request_thr);
    if (result == RESULT_ERROR) {
        udi_log("failed to handle command after thread create");
        udi_abort();
    }

    release_other_threads();

    return result;
}

int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *arg)
{
    udi_log("thread %a creating a new thread", get_user_thread_id());

    int block_result = block_other_threads();
    if (block_result < 0) {
        udi_log("failed to block other threads");
        udi_abort();
        return ENOMEM;
    }

    struct wrapped_thread_context *context =
        (struct wrapped_thread_context *)udi_malloc(sizeof(struct wrapped_thread_context));
    context->start_routine = start_routine;
    context->arg = arg;
    context->creator_tid = get_user_thread_id();

    int control_pipe[2];
    if ( init_control_pipe(control_pipe) != 0 ) {
        return ENOMEM;
    }

    int create_result = real_pthread_create(thread, attr, wrapped_start_routine, context);
    if (create_result == 0) {
        int handshake_result = handshake_with_thread();
        if (handshake_result != RESULT_SUCCESS) {
            return ENOMEM;
        }
    } else {
        if (get_multithread_capable()) {
            close(control_pipe[0]);
            close(control_pipe[1]);
        }
    }
    return create_result;
}

int get_num_threads() {
    return num_threads;
}

void destroy_thread(thread *thr) {

    thread *iter = threads;
    thread *last_thread = threads;
    while (iter != NULL) {
        if (iter == thr) break;

        last_thread = iter;
        iter = iter->next_thread;
    }

    if (iter == NULL) return;

    if (iter == threads) {
        threads = NULL;
    }else{
        last_thread->next_thread = iter->next_thread;
    }

    close(iter->control_write);
    close(iter->control_read);

    udi_free(iter->event_state.context_data);
    udi_free(iter);
    num_threads--;
}

udi_thread_state_e get_thread_state(thread *thr) {
    return thr->ts;
}

void set_thread_state(thread *thr, udi_thread_state_e state) {
    thr->ts = state;
}

uint64_t get_thread_id(thread *thr) {
    return thr->id;
}

int is_thread_context_valid(thread *thr) {
    return thr->event_state.context_valid;
}

void *get_thread_context(thread *thr) {
    return &thr->event_state.context;
}

int is_single_step(thread *thr) {
    return thr->single_step;
}

void set_single_step(thread *thr, int single_step) {
    thr->single_step = single_step;
}

breakpoint *get_single_step_breakpoint(thread *thr) {
    return thr->single_step_bp;
}

void set_single_step_breakpoint(thread *thr, breakpoint *bp) {
    thr->single_step_bp = bp;
}

thread *get_current_thread() {
    uint64_t tid = get_user_thread_id();

    thread *thr = threads;
    while (thr != NULL) {
        if (thr->id == tid) break;
        thr = thr->next_thread;
    }

    return thr;
}

thread *create_initial_thread() {
    return create_thread(get_user_thread_id());
}

static pthread_mutex_t log_lock;

int pthread_mutexattr_init(pthread_mutexattr_t *attr) UDI_WEAK;
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) UDI_WEAK;
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) UDI_WEAK;
int pthread_mutex_lock(pthread_mutex_t *mutex) UDI_WEAK;
int pthread_mutex_unlock(pthread_mutex_t *mutex) UDI_WEAK;

void init_thread_support() {
    if ( pthread_mutex_init && pthread_mutexattr_init ) {
        pthread_mutexattr_t attr;
        if ( pthread_mutexattr_init(&attr) != 0) {
            abort();
        }

        if ( pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0 ) {
            abort();
        }

        if ( pthread_mutex_init(&log_lock, &attr) != 0 ) {
            abort();
        }
    }
}

void udi_log_lock() {
    if ( pthread_mutex_lock ) {
        int lock_result = pthread_mutex_lock(&log_lock);
        if ( lock_result != 0 ) {
            abort();
        }
    }
}

void udi_log_unlock() {
    if ( pthread_mutex_unlock ) {
        int lock_result = pthread_mutex_unlock(&log_lock);
        if ( lock_result != 0 ) {
            abort();
        }
    }
}
