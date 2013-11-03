
#define _GNU_SOURCE 1

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <sys/syscall.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#include "bin_lib.h"

struct threadArg {
    int id;
    pthread_mutex_t *mutex;
    pthread_mutex_t *term_mutex;
};

struct threadArg threadStruct = { -1, NULL };

void usr1_handle(int sig) {
    long lwp_id = udi_get_thread_id(getpid());

    bin_printf("Received %d on thread %ld\n", sig, lwp_id);

    pause();
}

void breakpoint_func() {
    bin_printf("In function1\n");
}

void *entry(void *arg) {
    struct threadArg *thisArg = (struct threadArg *)arg;

    threadStruct = *thisArg;

    long lwp_id = udi_get_thread_id(getpid());
    bin_printf("%ld waiting on lock\n", lwp_id);

    if( pthread_mutex_lock(thisArg->mutex) != 0 ) {
        perror("pthread_mutex_lock");
        return NULL;
    }

    bin_printf("%ld obtained lock\n", lwp_id);

    if( pthread_mutex_unlock(thisArg->mutex) != 0 ) {
        perror("pthread_mutex_unlock");
        return NULL;
    }

    bin_printf("%ld released lock\n", lwp_id);

    breakpoint_func();

    bin_printf("%ld waiting on term lock\n", lwp_id);

    if ( pthread_mutex_lock(thisArg->term_mutex) != 0 ) {
        perror("pthread_mutex_lock");
        return NULL;
    }

    bin_printf("%ld obtained term lock\n", lwp_id);

    if ( pthread_mutex_unlock(thisArg->term_mutex) != 0 ) {
        perror("pthread_mutex_unlock");
        return NULL;
    }

    bin_printf("%ld released term lock\n", lwp_id);
    
    return NULL;
}

static const char *FIFO_NAME = "/tmp/waitthread-%d";

int main(int argc, char **argv) {
    init_bin();

    if( 2 != argc ) {
        printf("Usage: %s <num. of threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGUSR1, usr1_handle);

    bin_printf("%d\n", getpid());

    int numThreads;
    sscanf(argv[1], "%d", &numThreads);
    if( numThreads < 1 ) numThreads = 1;

    char fifo_name[1024];
    snprintf(fifo_name, 1024, FIFO_NAME, getpid());

    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t)*numThreads);

    pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    pthread_mutex_t *term_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    if( pthread_mutex_init(mutex, NULL) != 0 ) {
        perror("pthread_mutex_init");
        return EXIT_FAILURE;
    }

    if ( pthread_mutex_init(term_mutex, NULL) != 0 ) {
        perror("pthread_mutex_init");
        return EXIT_FAILURE;
    }

    if( pthread_mutex_lock(mutex) != 0 ) {
        perror("pthread_mutex_lock");
        return EXIT_FAILURE;
    }

    if( pthread_mutex_lock(term_mutex) != 0 ) {
        perror("pthread_mutex_lock");
        return EXIT_FAILURE;
    }

    int i;
    for(i = 0; i < numThreads; ++i) {
        struct threadArg *arg = (struct threadArg *)malloc(sizeof(struct threadArg));
        arg->id = i;
        arg->mutex = mutex;
        arg->term_mutex = term_mutex;
        assert( !pthread_create(&threads[i], NULL, &entry, (void *)arg) );
    }

    // breakpoint_func();

    // Wait on the named pipe
    unlink(fifo_name);
    if( mkfifo(fifo_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) ) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    FILE *fifo;
    do {
        if( (fifo = fopen(fifo_name, "r")) == NULL ) {
            if( errno == EINTR ) continue;

            perror("fopen");
            return EXIT_FAILURE;
        }
        break;
    }while(1);

    unsigned char byte;
    do {
        if( fread(&byte, sizeof(unsigned char), 1, fifo) != 1 ) {
            if ( errno == EINTR ) continue;

            if ( feof(fifo) ) {
                fprintf(stderr, "unexpected end of file\n");
            }else{
                perror("fread release");
            }

            return EXIT_FAILURE;
        }
        break;
    }while(1);

    bin_printf("Received notification\n");

    if( pthread_mutex_unlock(mutex) != 0 ) {
        perror("pthread_mutex_unlock");
        return EXIT_FAILURE;
    }

    bin_printf("Unlocked mutex, waiting for breakpoint notification\n");

    do {
        if( fread(&byte, sizeof(unsigned char), 1, fifo) != 1 ) {
            if ( errno == EINTR ) continue;

            if ( feof(fifo) ) {
                fprintf(stderr, "unexpected end of file\n");
            }else{
                perror("fread release");
            }

            return EXIT_FAILURE;
        }
        break;
    }while(1);

    bin_printf("Received notification\n");

    if ( pthread_mutex_unlock(term_mutex) != 0 ) {
        perror("pthread_mutex_unlock");
        return EXIT_FAILURE;
    }

    bin_printf("Unlocked term mutex, joining\n");

    for(i = 0; i < numThreads; ++i ) {
        assert( !pthread_join(threads[i], NULL) );
    }

    fclose(fifo);

    unlink(fifo_name);

    return EXIT_SUCCESS;
}
