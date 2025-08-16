#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../inc/MinHeapQueue.h"

// Funzioni per la sola gestione del Thread Pool
void threadpool_init(ThreadPool *pool, int num_threads);
void threadpool_add_task(ThreadPool *pool, void (*function)(void*), void* arg);
void threadpool_destroy(ThreadPool *pool);
void *worker_thread(void *arg);

#endif //THREAD_POOL_H