#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

// Definizione del semaforo binario
typedef struct bsem {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int v;
} bsem;

// Struttura per un singolo job
typedef struct job {
    void (*function)(void*);
    void* arg;
    long long priority;  // in our project is fileSize
    struct job* prev;
} job;

// linked list sorted in descending order
typedef struct jobqueue {
    job* front;
    job* rear;
    pthread_mutex_t rwmutex;
    bsem* has_jobs;
    int len;
} jobqueue;

// Struttura per un singolo thread del pool
typedef struct thread {
    int id;
    pthread_t pthread;
    struct ThreadPool* thpool_p;
} thread;

// Struttura principale del Thread Pool
typedef struct ThreadPool {
    thread** threads;
    int num_threads_alive;
    int num_threads_working;
    pthread_mutex_t thcount_lock;
    pthread_cond_t threads_all_idle;
    jobqueue jobqueue;
    int num_threads;
} ThreadPool;

// Prototipi delle funzioni
void threadpool_init(ThreadPool *pool, int num_threads);
void threadpool_add_job(ThreadPool *pool, void (*function)(void*), void* arg);
void threadpool_wait(ThreadPool *pool);
void threadpool_destroy(ThreadPool *pool);

// Funzioni di utilità per il semaforo
void bsem_init(bsem *b, int v);
void bsem_wait(bsem *b);
void bsem_post(bsem *b);

#endif // THREADPOOL_H
