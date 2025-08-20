#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "requestResponse.h"
#include "../inc/errExit.h"

char *path2ServerFIFO = "/tmp/fifoServer";
char *baseClientFIFO = "/tmp/fifoClient";

#define MAX 100
#define TIMEOUT_SECONDS 10

int main (int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Step-1: The client makes a FIFO in /tmp
    char path2ClientFIFO[25];
    sprintf(path2ClientFIFO, "%s%d", baseClientFIFO, getpid());

    printf("<Client> Starting client...\n");
    if (mkfifo(path2ClientFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
        errExit("mkfifo failed");

    printf("<Client> FIFO %s created\n", path2ClientFIFO);

    // Step-2: Open server FIFO to send request
    printf("<Client> Opening server FIFO %s...\n", path2ServerFIFO);
    int serverFIFO = open(path2ServerFIFO, O_WRONLY);
    if (serverFIFO == -1)
        errExit("Write-only server fifo opening failed");

    // Prepare a request
    struct Request request;
    request.cPid = getpid();

    // Copy filename from command line argument
    strncpy(request.fileName, argv[1], MAX_FILENAME_SIZE - 1);
    request.fileName[MAX_FILENAME_SIZE - 1] = '\0'; // Ensure null-termination

    // Step-3: Send request
    printf("<Client> Sending %s\n", request.fileName);
    if (write(serverFIFO, &request, sizeof(struct Request)) != sizeof(struct Request))
        errExit("Write-only client fifo opening failed");

    // Step-4: Open own FIFO for response
    int clientFIFO = open(path2ClientFIFO, O_RDONLY);
    if (clientFIFO == -1)
        errExit("Read-only client fifo opening failed");

    fd_set read_fds;
    struct timeval timeout;
    int retval;

    // Pulisce il set di file descriptor
    FD_ZERO(&read_fds);
    // Aggiunge il file descriptor del clientFIFO al set
    FD_SET(clientFIFO, &read_fds);

    // Imposta la struttura timeval per il timeout
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;

    printf("<Client> Waiting for a response (timeout: %d seconds)...\n", TIMEOUT_SECONDS);

    // Step-5: Wait for a response with a timeout
    retval = select(clientFIFO + 1, &read_fds, NULL, NULL, &timeout);

    if (retval == -1) {
        errExit("select() failed");
    } else if (retval) {
        printf("<Client> Data is available, reading response...\n");
        struct Response response;
        if (read(clientFIFO, &response, sizeof(struct Response)) != sizeof(struct Response)) {
            errExit("Server response reading failed");
        }
        // Step-6: Print server response
        printf("<Client> Server response: %s\n", response.hashCode);
    } else {
        // Timeout scaduto
        printf("<Client> Timeout occurred! No response from server after %d seconds.\n", TIMEOUT_SECONDS);
        // Esci dal programma con un codice di errore
        exit(EXIT_FAILURE);
    }

    // Step-7: Close FIFOs
    if (close(serverFIFO) != 0 || close(clientFIFO) != 0)
        errExit("Client fifo closing failed");

    // Step-8: Remove own FIFO
    if (unlink(path2ClientFIFO) != 0)
        errExit("Client fifo unlink failed");

    return 0;
}
