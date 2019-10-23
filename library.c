#include "library.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static struct itimerval timet;

static void schedule();

static void disable_timed_scheduler() {
    signal(SIGVTALRM, SIG_DFL);
}

static void enable_timed_scheduler() {
    signal(SIGVTALRM, schedule);
    setitimer(ITIMER_VIRTUAL, &timet, NULL);
}

static void free_thread_resources(tid id) {
    free(global_thread_queue.threads[id]->main_context->uc_stack.ss_sp);
    free(global_thread_queue.threads[id]->main_context);

    free(global_thread_queue.threads[id]->end_context->uc_stack.ss_sp);
    free(global_thread_queue.threads[id]->end_context);
}

static void schedule() {
    ut_thread_t *t;
    ut_thread_t *curr_t = global_thread_queue.current_running;

    uint64_t index = (curr_t->id + 1L) % (global_thread_queue.num_threads);

    do {

        t = global_thread_queue.threads[index];
        index = (index + 1) % (global_thread_queue.num_threads - 1);

    }
    while (t->finished == true);

    if(global_thread_queue.current_running->id != t->id) {
        global_thread_queue.current_running = t;
        enable_timed_scheduler();
        swapcontext(curr_t->main_context, t->main_context);
    }

}

static void ut_end(ut_thread_t *t, int status) {
    disable_timed_scheduler();
    //TODO: properly remove thread from queue
    global_thread_queue.num_active_threads--;
    global_thread_queue.threads[t->id]->finished = true;
    global_thread_queue.threads[t->id]->status = status;
    free_thread_resources(t->id);
    schedule();
}

int ut_init(void) {
    timet.it_interval.tv_sec = 0;
    timet.it_interval.tv_usec = 20000;
    timet.it_value.tv_sec = 0;
    timet.it_value.tv_usec = 20000;

    ut_thread_t *t = (ut_thread_t*) calloc(1,sizeof(ut_thread_t));
    t->main_context = (ucontext_t*) calloc(1, sizeof(ucontext_t));

    global_thread_queue.threads[0] = t;
    global_thread_queue.num_threads = 1;
    global_thread_queue.num_active_threads = 1;
    global_thread_queue.current_running = t;

    enable_timed_scheduler();
}

int ut_create(void (* entry)(void *), void* arg) {
    ut_thread_t *t = (ut_thread_t*) calloc(1,sizeof(ut_thread_t));
	//TODO: make and reuse ids
    t->id = global_thread_queue.num_threads;

    t->end_context = (ucontext_t*) calloc(1, sizeof(ucontext_t));
    t->end_stack = (char*) calloc(1, SIGSTKSZ);

    t->end_context->uc_link = NULL;
    t->end_context->uc_stack.ss_sp  = t->end_stack;
    t->end_context->uc_stack.ss_size = SIGSTKSZ;

    getcontext(t->end_context);
    makecontext(t->end_context, (void (*)(void)) ut_end, 2, t, EXIT_SUCCESS);

    t->main_context = (ucontext_t*) calloc(1, sizeof(ucontext_t));
    t->main_stack = (char*) calloc(1, SIGSTKSZ);

    t->main_context->uc_link = t->end_context;
    t->main_context->uc_stack.ss_sp = t->main_stack;
    t->main_context->uc_stack.ss_size = SIGSTKSZ;

    getcontext(t->main_context);
    makecontext(t->main_context, (void (*)(void)) entry, 1, arg);

    global_thread_queue.threads[global_thread_queue.num_threads] = t;
    global_thread_queue.num_threads++;
    global_thread_queue.num_active_threads++;

    return 1;
}

void ut_yield(void) {
    disable_timed_scheduler();
    schedule();
}

int ut_join(int tid, int * status) {
    while(!global_thread_queue.threads[tid]->finished);
    *status = global_thread_queue.threads[tid]->status;
	return 1;
}

void ut_exit(int status) {
    ut_end(global_thread_queue.current_running, status);
}

int ut_getid(void) {
    return global_thread_queue.current_running->id;
}
