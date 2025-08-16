#ifndef MINHEAPQUEUE_H
#define MINHEAPQUEUE_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_HEAP_SIZE 100

typedef struct {
    struct Request *data[MAX_HEAP_SIZE];
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} MinHeapQueue;

// Funzioni per la sola gestione della coda heap
void minheap_init(MinHeapQueue *queue);
void minheap_push(MinHeapQueue *queue, struct Request *request);
struct Request *minheap_pop(MinHeapQueue *queue);
void minheap_destroy(MinHeapQueue *queue);

#endif //MINHEAPQUEUE_H