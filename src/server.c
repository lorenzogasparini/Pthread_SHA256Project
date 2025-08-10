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
#include "../inc/request_response.h"

#define BUF_SIZE 8192

char *path2ServerFIFO ="/tmp/fifo_server";
char *baseClientFIFO = "/tmp/fifo_client.";

// The file descriptor entry for the FIFO
int serverFIFO, serverFIFO_extra;

void quit(int);
void * sendResponse(void * );           // Thread function
char * calculateFileHashCode(char *);   // TODO - Not useful by now 
char * process_file(void *);            // SHA256 processing function

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

    // Terminate the process
    _exit(0);
}

void * sendResponse(void *requestVoid) {
    // Retrieve and deallocate the pointer to the 'Request struct'
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
        return NULL;
    }

    // Prepare the response for the client
    struct Response response;
    strcpy(response.hashCode, process_file(request->fileName));

    // Write response into the client FIFO
    if (write(clientFIFO, &response,sizeof(struct Response)) != sizeof(struct Response)) {
        printf("<Server> Server fifo writing failed");
        free(request); // Free memory
        return NULL;
    }

    // Close the FIFO
    if (close(clientFIFO) != 0)
        printf("<Server> close failed");

    free(request); // Free memory
    return NULL;
}

char * process_file(void *arg) {
    char *filename = (char *)arg;
    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("Errore during file opening");
        free(filename);
        return NULL;
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

    //free(filename);
    return hashStr;
}

// TODO: implement a hash function - maybe not useful
char * calculateFileHashCode(char * fileName) {
    return "HashCodePlaceholder";
}

int main(int argc, char *argv[]) {
    printf("<Server> Starting client...\n");
    // Make a FIFO with the following permissions:
    // user:  read, write
    // group: write
    // other: no permission

    unlink(path2ServerFIFO);  // Remove the FIFO before creating it
    if (mkfifo(path2ServerFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
        errExit("mkfifo failed");

    printf("<Server> FIFO %s created\n", path2ServerFIFO);

    // Set a signal handler for SIGALRM and SIGINT signals // todo: serve?
    if (signal(SIGALRM, quit) == SIG_ERR ||
        signal(SIGINT, quit) == SIG_ERR)
    { errExit("Signal handlers setting failed"); }

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

        // Dynamic allocation of an empty request struct
        request = (struct Request *)malloc(sizeof(struct Request));
        if (request == NULL) {
            perror("Malloc failed");
            continue;
        }

        // Read a request from the FIFO and put it in request in heap
        // This is necessary because otherwise if we keep request in the stack the do-while loop constantly changes it
        // In this way each time a new request is allocated in the heap
        bR = read(serverFIFO, request, sizeof(struct Request));

        // Check the number of bytes read from the FIFO
        if (bR == -1) {
            printf("<Server> Something went wrong while reading request\n");
            free(request); // Free memory
        } else if (bR != sizeof(struct Request) || bR == 0) {
            printf("<Server> Bad request received\n");
            free(request); // Free memory
        } else {
            pthread_t thread;
            if (pthread_create(&thread, NULL, sendResponse, (void*)request) != 0) {
                perror("Error during thread creation");
                free(request); // Free memory
                exit(EXIT_FAILURE);
            }
            // By now, we keep a stand-alone thread processing (it's independent)
            // Thread processes the request on its own and then replies when finishes
            // It's not needed a join management by now. In future we have to manage a thread pool system 
            pthread_detach(thread);
        }

    } while (bR != -1);

    // FIFO is not working, run quit() to remove the FIFO and terminate the process 
    quit(0);
    return 0;
}
