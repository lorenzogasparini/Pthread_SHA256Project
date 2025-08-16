#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "../inc/threadPool.h"

/* Init Binary Semaphore */
void bsem_init(bsem *b, int v) {
    pthread_mutex_init(&(b->mutex), NULL);
    pthread_cond_init(&(b->cond), NULL);
    b->v = v;
}

/* Wait Binary Semaphore */
void bsem_wait(bsem *b) {
    pthread_mutex_lock(&(b->mutex));
    while (b->v == 0) {
        pthread_cond_wait(&(b->cond), &(b->mutex));
    }
    b->v--;
    pthread_mutex_unlock(&(b->mutex));
}

/* Signal there is a available job */
void bsem_post(bsem *b) {
    pthread_mutex_lock(&(b->mutex));
    b->v++;
    pthread_cond_signal(&(b->cond));
    pthread_mutex_unlock(&(b->mutex));
}

/* Init Job Queue */
static void jobqueue_init(jobqueue *jobqueue) {
    jobqueue->len = 0;
    jobqueue->front = NULL;
    jobqueue->rear = NULL;
    pthread_mutex_init(&(jobqueue->rwmutex), NULL);
    jobqueue->has_jobs = (bsem*)malloc(sizeof(bsem));
    if (jobqueue->has_jobs == NULL) {
        perror("Error during allocation bsem");
        exit(1);
    }
    bsem_init(jobqueue->has_jobs, 0);
}

/* Empty the queue and free the memory */
static void jobqueue_destroy(jobqueue *jobqueue) {
    job* curr_job = jobqueue->front;
    while(curr_job != NULL) {
        job* next_job = curr_job->prev;
        free(curr_job);
        curr_job = next_job;
    }
    pthread_mutex_destroy(&(jobqueue->rwmutex));
    free(jobqueue->has_jobs);
}

/* Extract a job from the queue */
static job* jobqueue_pull(jobqueue *jobqueue) {
    pthread_mutex_lock(&(jobqueue->rwmutex));
    job* job = jobqueue->front;

    if(job == NULL) {
        pthread_mutex_unlock(&(jobqueue->rwmutex));
        return NULL;
    }

    // Remove a job from the queue
    jobqueue->front = job->prev;
    jobqueue->len--;

    // If queue is empty, update rear pointer
    if (jobqueue->len == 0) {
        jobqueue->rear = NULL;
    }

    pthread_mutex_unlock(&(jobqueue->rwmutex));
    return job;
}

/* Thread execution function */
void *worker_thread(void *arg) {
    thread* self = (thread*)arg;
    ThreadPool* thpool_p = self->thpool_p;

    pthread_mutex_lock(&(thpool_p->thcount_lock));
    thpool_p->num_threads_alive++;
    pthread_mutex_unlock(&(thpool_p->thcount_lock));

    while(1) {
        bsem_wait(thpool_p->jobqueue.has_jobs);

        if (thpool_p->num_threads_alive == 0) {
            break;
        }

        pthread_mutex_lock(&(thpool_p->thcount_lock));
        thpool_p->num_threads_working++;
        pthread_mutex_unlock(&(thpool_p->thcount_lock));

        job* current_job = jobqueue_pull(&(thpool_p->jobqueue));

        if (current_job) {
            current_job->function(current_job->arg);
            free(current_job);
        }

        pthread_mutex_lock(&(thpool_p->thcount_lock));
        thpool_p->num_threads_working--;

        // If the queue is empty, send a signal to all thread 
        if (thpool_p->jobqueue.len == 0 && thpool_p->num_threads_working == 0) {
            pthread_cond_signal(&(thpool_p->threads_all_idle));
        }
        pthread_mutex_unlock(&(thpool_p->thcount_lock));
    }

    pthread_mutex_lock(&(thpool_p->thcount_lock));
    thpool_p->num_threads_alive--;
    pthread_mutex_unlock(&(thpool_p->thcount_lock));

    return NULL;
}

// Init Thread Pool
void threadpool_init(ThreadPool *pool, int num_threads) {
    if (num_threads < 1) {
        num_threads = 1;
    }

    // Allocate memory for threads in the thread pool
    pool->threads = (thread**)malloc(sizeof(thread*) * num_threads);
    if (pool->threads == NULL) {
        perror("Error during thread allocation");
        exit(1);
    }

    pool->num_threads_alive = 0;
    pool->num_threads_working = 0;

    jobqueue_init(&(pool->jobqueue));
    pthread_mutex_init(&(pool->thcount_lock), NULL);
    pthread_cond_init(&(pool->threads_all_idle), NULL);

    // Thread creation in thread pool
    for (int i = 0; i < num_threads; i++) {
        pool->threads[i] = (thread*)malloc(sizeof(thread));
        if (pool->threads[i] == NULL) {
            perror("Error during thread allocation");
            exit(1);
        }
        pool->threads[i]->thpool_p = pool;
        pool->threads[i]->id = i;
        pthread_create(&(pool->threads[i]->pthread), NULL, worker_thread, pool->threads[i]);
    }
}

// Add a job to thread pool
void threadpool_add_job(ThreadPool *pool, void (*function)(void*), void* arg) {
    job* new_job = (job*)malloc(sizeof(job));
    if (new_job == NULL) {
        perror("Error during job allocation");
        return;
    }
    new_job->function = function;
    new_job->arg = arg;
    new_job->prev = NULL;

    pthread_mutex_lock(&(pool->jobqueue.rwmutex));

    if (pool->jobqueue.len == 0) {
        pool->jobqueue.front = new_job;
        pool->jobqueue.rear = new_job;
    } else {
        pool->jobqueue.rear->prev = new_job;
        pool->jobqueue.rear = new_job;
    }
    pool->jobqueue.len++;

    pthread_mutex_unlock(&(pool->jobqueue.rwmutex));

    // Signal there is an available job 
    bsem_post(pool->jobqueue.has_jobs);
}

// Block the calling thread until all the jobs are completed
void threadpool_wait(ThreadPool *pool) {
    pthread_mutex_lock(&(pool->thcount_lock));
    while (pool->jobqueue.len > 0 || pool->num_threads_working > 0) {
        pthread_cond_wait(&(pool->threads_all_idle), &(pool->thcount_lock));
    }
    pthread_mutex_unlock(&(pool->thcount_lock));
}

// Empty the thread pool and free the resources allocated
void threadpool_destroy(ThreadPool *pool) {
    if (pool == NULL) {
        return;
    }

    // Wait for all threads to finish
    threadpool_wait(pool);

    // Set the number of active threads to 0 to signal exit
    pthread_mutex_lock(&(pool->thcount_lock));
    pool->num_threads_alive = 0;
    pthread_mutex_unlock(&(pool->thcount_lock));

    // Wakeup all waiting threads
    for (int i = 0; i < pool->num_threads_alive + pool->num_threads_working; i++) {
        bsem_post(pool->jobqueue.has_jobs);
    }

    // Join all threads
    for (int i = 0; i < pool->num_threads_alive; i++) {
        pthread_join(pool->threads[i]->pthread, NULL);
    }

    // Free allocated memory
    jobqueue_destroy(&(pool->jobqueue));
    for (int i = 0; i < pool->num_threads_alive; i++) {
        free(pool->threads[i]);
    }
    free(pool->threads);

    pthread_mutex_destroy(&(pool->thcount_lock));
    pthread_cond_destroy(&(pool->threads_all_idle));

    printf("Thread pool destroyed succesfully\n");
}

// Sample function for a thread task
void simple_task(void* arg) {
    int task_id = *(int*)arg;
    printf("\nExecuting task %d on thread %lu\n", task_id, (unsigned long)pthread_self());
    usleep(100000);
    printf("Completed task %d\n", task_id);
    free(arg);
}