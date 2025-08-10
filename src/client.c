#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "request_response.h"
#include "../inc/errExit.h"

char *path2ServerFIFO = "/tmp/fifo_server";
char *baseClientFIFO = "/tmp/fifo_client.";

#define MAX 100

int main (int argc, char *argv[]) {

    // Step-1: The client makes a FIFO in /tmp
    char path2ClientFIFO [25];
    sprintf(path2ClientFIFO, "%s%d", baseClientFIFO, getpid());

    printf("<Client> Starting server...\n");
    // make a FIFO with the following permissions:
    // user:  read, write
    // group: write
    // other: no permission
    if (mkfifo(path2ClientFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
        errExit("mkfifo failed");

    printf("<Client> FIFO %s created\n", path2ClientFIFO);

    // Step-2: The client opens the server's FIFO to send a request
    printf("<Client> Opening server FIFO %s...\n", path2ServerFIFO);
    int serverFIFO = open(path2ServerFIFO, O_WRONLY);
    if (serverFIFO == -1)
        errExit("Write-only server fifo opening failed");

    // Prepare a request
    struct Request request;
    request.cPid = getpid();
    // Get the filename from the user input
    printf("Insert a valid file path (max %d characters): ", MAX_FILENAME_SIZE - 1);
    char input_filename[MAX_FILENAME_SIZE];
    if (fgets(input_filename, MAX_FILENAME_SIZE, stdin) == NULL)
        errExit("Fgets failed");

    input_filename[strcspn(input_filename, "\n")] = 0; // Remove '\n' from the input
    // Copy the filename into the struct
    strcpy(request.fileName, input_filename);

    // Step-3: The client sends a Request through the server's FIFO
    printf("<Client> Sending %s\n", request.fileName);
    if (write(serverFIFO, &request,
        sizeof(struct Request)) != sizeof(struct Request))
        errExit("Write-only client fifo opening failed");

    // Step-4: The client opens its FIFO to get a response
    int clientFIFO = open(path2ClientFIFO, O_RDONLY);
    if (clientFIFO == -1)
        errExit("Read-only client fifo opening failed");

    // Step-5: The client reads a response from the server
    struct Response response;
    if (read(clientFIFO, &response, // Wait a response
        sizeof(struct Response)) != sizeof(struct Response))
        errExit("Server response reading failed");

    // Step-6: The client prints the result on terminal
    printf("<Client> Server response: %s\n", response.hashCode);

    // Step-7: The client closes its FIFO
    if (close(serverFIFO) != 0 || close(clientFIFO) != 0)
        errExit("Client fifo closing failed");

    // Step-8: The client removes its FIFO from the file system
    if (unlink(path2ClientFIFO) != 0)
        errExit("Client fifo unlink failed");
}
