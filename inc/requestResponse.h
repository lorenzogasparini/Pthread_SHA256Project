#ifndef _REQUEST_RESPONSE_HH
#define _REQUEST_RESPONSE_HH
#include <sys/types.h>

#define MAX_FILENAME_SIZE 256  /* Massima dimensione del nome del file */

struct Request {          /* Request (client --> server)  */
    pid_t cPid;           /* PID of client                */
    char fileName[MAX_FILENAME_SIZE]; /* Nome del file */
};

struct Response {         /* Response (server --> client) */
    char hashCode[256];   /* file HASH using SHA-256      */
};

#endif
