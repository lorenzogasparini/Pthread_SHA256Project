#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../inc/errExit.h"
#include "../inc/request_response.h"

char *path2ServerFIFO ="/tmp/fifo_server";
char *baseClientFIFO = "/tmp/fifo_client.";

// the file descriptor entry for the FIFO
int serverFIFO, serverFIFO_extra;

// firme dei metodi:
void quit(int);
void * sendResponse(void * ); // thread function
char * calculateFileHashCode(char *);


// the quit function closes the file descriptors for the FIFO,
// removes the FIFO from the file system, and terminates the process
void quit(int sig) {
    // the quit function is used as signal handler for the ALARM signal too.
    if (sig == SIGALRM)
        printf("<Server> Time expired!\n");

    // Close the FIFO
    if (serverFIFO != 0 && close(serverFIFO) == -1)
        errExit("close failed");

    if (serverFIFO_extra != 0 && close(serverFIFO_extra) == -1)
        errExit("close failed");

    // Remove the FIFO
    if (unlink(path2ServerFIFO) != 0)
        errExit("unlink failed");

    // terminate the process
    _exit(0);
}

void * sendResponse(void *requestVoid) {

    // Recupera e dereferenzia il puntatore alla struct Request
    struct Request * request = (struct Request *) requestVoid;

    // make the path of client's FIFO
    char path2ClientFIFO [25];
    sprintf(path2ClientFIFO, "%s%d", baseClientFIFO, request->cPid);

    printf("<Server> opening FIFO %s...\n", path2ClientFIFO);
    // Open the client's FIFO in write-only mode
    int clientFIFO = open(path2ClientFIFO, O_WRONLY);
    if (clientFIFO == -1) {
        printf("<Server> open failed");
        free(request); // Libera la memoria allocata
        return NULL;
    }

    // Prepare the response for the client
    struct Response response;
    strcpy(response.hashCode, calculateFileHashCode(request->fileName));

    printf("<Server> fileName: %s, hashCode: %s\n", request->fileName, response.hashCode);
    // Write the Response into the opened FIFO
    if (write(clientFIFO, &response,sizeof(struct Response)) != sizeof(struct Response)) {
        printf("<Server> write failed");
        free(request); // Libera la memoria allocata
        return NULL;
    }

    // Close the FIFO
    if (close(clientFIFO) != 0)
        printf("<Server> close failed");

    free(request); // Libera la memoria allocata
    return NULL;
}

// todo: implement hashCode Function
char * calculateFileHashCode(char * fileName) {
    // call hashCodeFunction
    return "HashCodePlaceholder";
}

int main (int argc, char *argv[]) {

    printf("<Server> Making FIFO...\n");
    // make a FIFO with the following permissions:
    // user:  read, write
    // group: write
    // other: no permission

    unlink(path2ServerFIFO);  // remove the FIFO before creating it
    if (mkfifo(path2ServerFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
        errExit("mkfifo failed");

    printf("<Server> FIFO %s created!\n", path2ServerFIFO);

    // set a signal handler for SIGALRM and SIGINT signals // todo: serve?
    if (signal(SIGALRM, quit) == SIG_ERR ||
        signal(SIGINT, quit) == SIG_ERR)
    { errExit("change signal handlers failed"); }


    // Wait for client in read-only mode. The open blocks the calling process
    // until another process opens the same FIFO in write-only mode
    printf("<Server> waiting for a client...\n");
    serverFIFO = open(path2ServerFIFO, O_RDONLY);
    if (serverFIFO == -1)
        errExit("open read-only failed");

    // Open an extra descriptor, so that the server does not see end-of-file
    // even if all clients closed the write end of the FIFO
    serverFIFO_extra = open(path2ServerFIFO, O_WRONLY);
    if (serverFIFO_extra == -1)
        errExit("open write-only failed");

    struct Request *request;
    int bR = -1;
    do {
        printf("<Server> waiting for a Request...\n");

        // Alloca dinamicamente memoria per la richiesta
        request = (struct Request *)malloc(sizeof(struct Request));
        if (request == NULL) {
            perror("malloc failed");
            continue; // Continua il loop in caso di errore di allocazione
        }

        // Read a request from the FIFO and put it in request in heap
        // this is necessary because otherwise if we keep request in the stack the do-while
        // loop constantly changes it. In this way each time a new request is allocated in
        // the heap
        bR = read(serverFIFO, request, sizeof(struct Request));

        // Check the number of bytes read from the FIFO
        if (bR == -1) {
            printf("<Server> it looks like the FIFO is broken\n");
            free(request); // Libera la memoria
        } else if (bR != sizeof(struct Request) || bR == 0) {
            printf("<Server> it looks like I did not receive a valid request\n");
            free(request); // Libera la memoria
        } else {
            pthread_t thread;
            if (pthread_create(&thread, NULL, sendResponse, (void*)request) != 0) {
                perror("Errore nella creazione del thread");
                free(request); // Libera la memoria
                exit(EXIT_FAILURE);
            }
            // Non è necessario fare il join qui, il thread è indipendente.
            // In un thread pool reale, si userebbe una coda di lavoro.
            // Per il momento, possiamo anche creare un thread staccato
            // in modo che non si debba fare il join.
            // Se si stacca, non si deve fare il join.
            pthread_detach(thread);
        }

    } while (bR != -1);

    // the FIFO is broken, run quit() to remove the FIFO and
    // terminate the process.
    quit(0);
    return 0;
}
