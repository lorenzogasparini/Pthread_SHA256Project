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

    // Step-5: Read response
    struct Response response;
    if (read(clientFIFO, &response, sizeof(struct Response)) != sizeof(struct Response))
        errExit("Server response reading failed");

    // Step-6: Print server response
    printf("<Client> Server response: %s\n", response.hashCode);

    // Step-7: Close FIFOs
    if (close(serverFIFO) != 0 || close(clientFIFO) != 0)
        errExit("Client fifo closing failed");

    // Step-8: Remove own FIFO
    if (unlink(path2ClientFIFO) != 0)
        errExit("Client fifo unlink failed");

    return 0;
}
