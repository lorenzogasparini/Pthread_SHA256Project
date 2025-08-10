#include "errExit.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

/*
    Handling method for manage a generic error message
*/
void errExit(const char *msg) {
    perror(msg);
    exit(1);
}
