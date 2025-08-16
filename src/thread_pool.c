#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // Per la funzione sleep()

/* Semaforo binario contatore bsem */
typedef struct bsem {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int v;
} bsem;

/* job */
typedef struct job{
    struct job* prev;
    void (*function)(void* arg);
    void* arg;
} job;

/* Coda dei job */
typedef struct jobqueue{
    pthread_mutex_t rwmutex;
    job *front;
    job *rear;
    bsem *has_jobs;
    int len;
} jobqueue;

/* Thread worker */
typedef struct thread{
    int id;
    pthread_t pthread;
    struct thpool_* thpool_p;
} thread;

/* Threadpool */
typedef struct thpool_{
    thread** threads;
    volatile int num_threads_alive;
    volatile int num_threads_working;
    pthread_mutex_t thcount_lock;
    pthread_cond_t threads_all_idle;
    jobqueue jobqueue;
} thpool_;

typedef struct thpool_ ThreadPool;

// Inizializza un semaforo binario, bsem, contatore
void bsem_init(bsem *b, int v) {
    pthread_mutex_init(&(b->mutex), NULL);
    pthread_cond_init(&(b->cond), NULL);
    b->v = v;
}

// Operazione wait su semaforo binario bsem
void bsem_wait(bsem *b) {
    pthread_mutex_lock(&(b->mutex));
    while (b->v == 0) {
        pthread_cond_wait(&(b->cond), &(b->mutex));
    }
    b->v--;
    pthread_mutex_unlock(&(b->mutex));
}

void bsem_post(bsem *b) {
    pthread_mutex_lock(&(b->mutex));
    b->v++;
    pthread_cond_signal(&(b->cond));
    pthread_mutex_unlock(&(b->mutex));
}

// Inizializza la coda dei job
static void jobqueue_init(jobqueue *jobqueue) {
    jobqueue->len = 0;
    jobqueue->front = NULL;
    jobqueue->rear = NULL;
    pthread_mutex_init(&(jobqueue->rwmutex), NULL);
    jobqueue->has_jobs = (bsem*)malloc(sizeof(bsem));
    if (jobqueue->has_jobs == NULL) {
        perror("Errore di allocazione per bsem");
        exit(1);
    }
    bsem_init(jobqueue->has_jobs, 0);
}

// Distrugce la coda dei job e libera la memoria
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

// Estrae un job dalla coda
static job* jobqueue_pull(jobqueue *jobqueue) {
    pthread_mutex_lock(&(jobqueue->rwmutex));
    job* job = jobqueue->front;

    if(job == NULL) {
        pthread_mutex_unlock(&(jobqueue->rwmutex));
        return NULL;
    }

    // Rimuove il job dalla coda
    jobqueue->front = job->prev;
    jobqueue->len--;

    // Se la coda è vuota, aggiorna il puntatore rear
    if (jobqueue->len == 0) {
        jobqueue->rear = NULL;
    }

    pthread_mutex_unlock(&(jobqueue->rwmutex));
    return job;
}

//  Funzione eseguita da ogni thread
void *worker_thread(void *arg) {
    thread* self = (thread*)arg;
    ThreadPool* thpool_p = self->thpool_p;

    // Incrementa il conteggio dei thread attivi
    pthread_mutex_lock(&(thpool_p->thcount_lock));
    thpool_p->num_threads_alive++;
    pthread_mutex_unlock(&(thpool_p->thcount_lock));

    while(1) {
        bsem_wait(thpool_p->jobqueue.has_jobs);

        // Verifica distruzione threadpool
        if (thpool_p->num_threads_alive == 0) {
            break;
        }

        // Estrazione job dalla coda
        pthread_mutex_lock(&(thpool_p->thcount_lock));
        thpool_p->num_threads_working++;
        pthread_mutex_unlock(&(thpool_p->thcount_lock));

        job* current_job = jobqueue_pull(&(thpool_p->jobqueue));

        if (current_job) {
            // Esecuzione lavoro
            current_job->function(current_job->arg);
            free(current_job);
        }

        // Aggiorna il valore dei thread che lavorano
        pthread_mutex_lock(&(thpool_p->thcount_lock));
        thpool_p->num_threads_working--;

        // Se la coda è vuota e non ci sono più thread che lavorano, invia un segnale --> DA VERIFICARE
        if (thpool_p->jobqueue.len == 0 && thpool_p->num_threads_working == 0) {
            pthread_cond_signal(&(thpool_p->threads_all_idle));
        }
        pthread_mutex_unlock(&(thpool_p->thcount_lock));
    }

    // Decrementa il conteggio dei thread attivi prima di uscire
    pthread_mutex_lock(&(thpool_p->thcount_lock));
    thpool_p->num_threads_alive--;
    pthread_mutex_unlock(&(thpool_p->thcount_lock));

    return NULL;
}

//  Inizializza il thread pool
void threadpool_init(ThreadPool *pool, int num_threads) {
    if (num_threads < 1) {
        num_threads = 1;
    }

    // Alloca memoria per i thread del thread pool
    pool->threads = (thread**)malloc(sizeof(thread*) * num_threads);
    if (pool->threads == NULL) {
        perror("Errore di allocazione per i thread");
        exit(1);
    }

    pool->num_threads_alive = 0;
    pool->num_threads_working = 0;

    // Inizializza la coda dei job
    jobqueue_init(&(pool->jobqueue));

    // Inizializza i mutex e le variabili di condizione
    pthread_mutex_init(&(pool->thcount_lock), NULL);
    pthread_cond_init(&(pool->threads_all_idle), NULL);

    // Crea i thread worker
    for (int i = 0; i < num_threads; i++) {
        pool->threads[i] = (thread*)malloc(sizeof(thread));
        if (pool->threads[i] == NULL) {
            perror("Errore di allocazione per il thread");
            exit(1);
        }
        pool->threads[i]->thpool_p = pool;
        pool->threads[i]->id = i;
        pthread_create(&(pool->threads[i]->pthread), NULL, worker_thread, pool->threads[i]);
    }
}

//  Aggiunge un job al thread pool
void threadpool_add_task(ThreadPool *pool, void (*function)(void*), void* arg) {
    job* new_job = (job*)malloc(sizeof(job));
    if (new_job == NULL) {
        perror("Errore di allocazione per il job");
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

    // Segnala che c'è un nuovo job disponibile
    bsem_post(pool->jobqueue.has_jobs);
}

//  Blocca il thread chiamante finché tutti i job non sono completati.
void threadpool_wait(ThreadPool *pool) {
    pthread_mutex_lock(&(pool->thcount_lock));
    while (pool->jobqueue.len > 0 || pool->num_threads_working > 0) {
        pthread_cond_wait(&(pool->threads_all_idle), &(pool->thcount_lock));
    }
    pthread_mutex_unlock(&(pool->thcount_lock));
}

//  Distrugge il thread pool e libera tutte le risorse
void threadpool_destroy(ThreadPool *pool) {
    if (pool == NULL) {
        return;
    }

    // Prima di distruggere, attendiamo che tutti i task siano completati
    threadpool_wait(pool);

    // Imposta il numero di thread attivi a 0 per segnalare l'uscita
    pthread_mutex_lock(&(pool->thcount_lock));
    pool->num_threads_alive = 0;
    pthread_mutex_unlock(&(pool->thcount_lock));

    // Sveglia tutti i thread in attesa
    for (int i = 0; i < pool->num_threads_alive + pool->num_threads_working; i++) {
        bsem_post(pool->jobqueue.has_jobs);
    }

    // Attendi che tutti i thread terminino
    for (int i = 0; i < pool->num_threads_alive; i++) {
        pthread_join(pool->threads[i]->pthread, NULL);
    }

    // Libera la memoria allocata
    jobqueue_destroy(&(pool->jobqueue));

    for (int i = 0; i < pool->num_threads_alive; i++) {
        free(pool->threads[i]);
    }
    free(pool->threads);

    pthread_mutex_destroy(&(pool->thcount_lock));
    pthread_cond_destroy(&(pool->threads_all_idle));

    printf("Thread pool distrutto con successo.\n");
}


// Funzione di esempio per un job
void simple_task(void* arg) {
    int task_id = *(int*)arg;
    printf("\nEsecuzione del task %d su thread %lu\n", task_id, (unsigned long)pthread_self());
    printf("Test! -> %d\n", arg);
    usleep(100000);
    printf("Completato il task %d\n", task_id);
    free(arg);
}

int main() {
    ThreadPool my_pool;
    int num_threads = 4;
    int num_tasks = 20;

    printf("Inizializzazione del thread pool con %d thread...\n", num_threads);
    threadpool_init(&my_pool, num_threads);

    printf("Aggiunta di %d task al thread pool...\n", num_tasks);
    for (int i = 0; i < num_tasks; i++) {
        int* task_id = (int*)malloc(sizeof(int));
        if (task_id == NULL) {
            perror("Errore di allocazione per task_id");
            exit(1);
        }
        *task_id = i + 1;
        threadpool_add_task(&my_pool, simple_task, task_id);
    }

    // Attendi che tutti i task siano completati
    threadpool_wait(&my_pool);

    printf("Tutti i task sono stati completati.\n");

    printf("Distruzione del thread pool...\n");
    threadpool_destroy(&my_pool);

    return 0;
}
