#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Binary semaphore */
typedef struct bsem {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int v;
} bsem;

/* Job */
typedef struct job{
    struct job* prev;
    void (*function)(void* arg);
    void* arg;
} job;

/* Job Queue */
typedef struct jobqueue{
    pthread_mutex_t rwmutex;
    job *front;
    job *rear;
    bsem *has_jobs;
    int len;
} jobqueue;

/* Thread Worker */
typedef struct thread{
    int id;
    pthread_t pthread;
    struct thpool_* thpool_p;
} thread;

/* Thread Pool */
typedef struct thpool_{
    thread** threads;
    volatile int num_threads_alive;
    volatile int num_threads_working;
    pthread_mutex_t thcount_lock;
    pthread_cond_t threads_all_idle;
    jobqueue jobqueue;
} thpool_;

typedef struct thpool_ ThreadPool;

void threadpool_init(ThreadPool *pool, int num_threads);
void threadpool_add_job(ThreadPool *pool, void (*function)(void*), void* arg);
void threadpool_destroy(ThreadPool *pool);
void *worker_thread(void *arg);

#endif