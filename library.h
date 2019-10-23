#ifndef USER_THREADS_LIBRARY_H
#define USER_THREADS_LIBRARY_H

#include <stdint.h>
#include <ucontext.h>
#include <bits/sigstack.h>
#include <stdbool.h>

typedef uint64_t tid;


typedef struct ut_thread {
    tid id;
    ucontext_t *main_context;
    ucontext_t *end_context;

    char *main_stack;
    char *end_stack;

    bool finished;
    int status;

    void (*entry)(void *);

} ut_thread_t;

//TODO: change this to a proper queue
static struct thread_queue {
    ut_thread_t *threads[1000];
    uint64_t num_threads;
    uint64_t num_active_threads;
    ut_thread_t * current_running;

} global_thread_queue;

int ut_init(void);
int ut_create(void (* entry)(void *), void* arg);
int ut_join(int tid, int * status);
void ut_exit(int status);
void ut_yield(void);
int ut_getid(void);

#endif //USER_THREADS_LIBRARY_H
