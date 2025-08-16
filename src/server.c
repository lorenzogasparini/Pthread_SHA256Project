#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <openssl/sha.h>

#include "../inc/errExit.h"
#include "../inc/requestResponse.h"
#include "../inc/threadPool.h"
#include "../inc/hashTable.h"

#define BUF_SIZE 8192
#define NUM_THREAD 4 
#define EMPTY_STRING ""

char *path2ServerFIFO = "/tmp/fifoServer";
char *baseClientFIFO = "/tmp/fifoClient";

// The file descriptor entry for the FIFO
int serverFIFO, serverFIFO_extra;

// Cache for already calculated hashes
HashTable *cache;

void quit(int);                                 // Exit function
void processRequest(void * );                   // Thread function
char *SHA256_hashFile(void *);                  // SHA256 processing function

// The quit function closes the file descriptors for the FIFO,
// Removes the FIFO from the file system, and terminates the process
void quit(int sig) {
    // The quit function is used as signal handler for the ALARM signal too.
    if (sig == SIGALRM)
        printf("<Server> Time expired!\n");

    // Close the FIFO
    if (serverFIFO != 0 && close(serverFIFO) == -1)
        errExit("Server fifo closing failed");

    if (serverFIFO_extra != 0 && close(serverFIFO_extra) == -1)
        errExit("Extra server fifo closing failed");

    // Remove the FIFO
    if (unlink(path2ServerFIFO) != 0)
        errExit("Server fifo unlink failed");

    // Free hash table
    if (cache)
        free_hash_table(cache);

    // Terminate the process
    _exit(0);
}

void processRequest(void *requestVoid) {
    // Retrieve and deallocate the pointer to the request
    struct Request * request = (struct Request *) requestVoid;

    // Make the path of client's FIFO
    char path2ClientFIFO [25];
    sprintf(path2ClientFIFO, "%s%d", baseClientFIFO, request->cPid);

    printf("<Server> Opening FIFO %s...\n", path2ClientFIFO);
    // Open the client's FIFO in write-only mode
    int clientFIFO = open(path2ClientFIFO, O_WRONLY);
    if (clientFIFO == -1) {
        printf("<Server> Client fifo opening failed");
        free(request); // Free memory
    }

    // Preparing response for the client
    struct Response response;

    // Check if the cache already got the related hash value
    char *cachedHash = hash_table_get(cache, request->fileName);
    if (cachedHash) {
        strcpy(response.hashCode, cachedHash);
        printf("<Server> Cache hit for file '%s'\n", request->fileName);
    } else {
        char *hash = SHA256_hashFile(request->fileName);
        if (!hash) hash = (char *)EMPTY_STRING;

        strcpy(response.hashCode, hash);
        hash_table_insert(cache, request->fileName, hash);
        free(hash);
    }

    // Write response into the client FIFO
    if (write(clientFIFO, &response, sizeof(struct Response)) != sizeof(struct Response))
        printf("<Server> Server fifo writing failed\n");

    // Close FIFO
    if (close(clientFIFO) != 0)
        printf("<Server> close failed");

    free(request);
}

char *SHA256_hashFile(void *arg) {
    char *filename = (char *)arg;
    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("Error during file opening");
        return EMPTY_STRING;
    }

    unsigned char buffer[BUF_SIZE];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;

    SHA256_Init(&sha256);
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUF_SIZE, file)) > 0) {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);
    fclose(file);

    // Allocate the digest in a dynamic memory space, so we can manage it outside of this function
    char *hashStr = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    if (!hashStr) return NULL;

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hashStr + (i * 2), "%02x", hash[i]);
    }
    hashStr[SHA256_DIGEST_LENGTH * 2] = '\0';

    printf("<Server> Thread [%lu] - SHA256 Digest generation of path '%s':\n<Server> Digest created: %s\n",
           pthread_self(), filename, hashStr);

    return hashStr;
}

int main(int argc, char *argv[]) {
    ThreadPool my_pool;
    int task_id = 1;

    printf("<Server> Starting server...\n");
    // Make a FIFO with the following permissions:
    // user:  read, write
    // group: write
    // other: no permission

    // Remove the FIFO before creating it
    unlink(path2ServerFIFO); 
    
    if (mkfifo(path2ServerFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
        errExit("mkfifo failed");
    printf("<Server> FIFO %s created\n", path2ServerFIFO);

    // Set a signal handler for SIGALRM and SIGINT signals
    if (signal(SIGALRM, quit) == SIG_ERR ||
        signal(SIGINT, quit) == SIG_ERR)
    { errExit("Signal handlers setting failed"); }

    // Initialize thread pool
    printf("Initializing thread pool with %d thread...\n", NUM_THREAD);
    threadpool_init(&my_pool, NUM_THREAD);

    // Hash table creation
    cache = create_hash_table();

    // Wait for client in read-only mode. The open blocks the calling process
    // until another process opens the same FIFO in write-only mode
    printf("<Server> Waiting for a client...\n");
    serverFIFO = open(path2ServerFIFO, O_RDONLY);
    if (serverFIFO == -1)
        errExit("Read-only server fifo opening failed");

    // Open an extra descriptor, so that the server does not see end-of-file
    // even if all clients closed the write end of the FIFO
    serverFIFO_extra = open(path2ServerFIFO, O_WRONLY);
    if (serverFIFO_extra == -1)
        errExit("Write-only server fifo opening failed");

    struct Request *request;
    int bR = -1;
    do {
        printf("<Server> Waiting for a request...\n");

        // Dynamic allocation of an empty request
        request = (struct Request *)malloc(sizeof(struct Request));
        if (request == NULL) {
            perror("Request allocation failed");
            continue;
        }

        // Read a request from the FIFO and put it in request in heap
        // This is necessary because otherwise if we keep request in the stack the do-while loop constantly changes it
        // In this way each time a new request is allocated in the heap
        bR = read(serverFIFO, request, sizeof(struct Request));
        
        // Check the number of bytes read from the FIFO
        if (bR == -1) {
            printf("<Server> Something went wrong while reading request (task_id=%d)\n", task_id);
            free(request);
        } else if (bR != sizeof(struct Request) || bR == 0) {
            printf("<Server> Bad request received (task_id=%d)\n", task_id);
            free(request);
        } else {
            printf("<Server> Forward request to a separate thread (task_id=%d)...\n", task_id);
            threadpool_add_job(&my_pool, processRequest, request);
        }
        task_id++;
    } while (bR != -1);

    threadpool_wait(&my_pool);
    threadpool_destroy(&my_pool);
    quit(0);
    return 0;
}
