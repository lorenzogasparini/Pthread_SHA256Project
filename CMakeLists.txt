cmake_minimum_required(VERSION 3.10)
project(Pthread_SHA256Project C)

set(CMAKE_C_STANDARD 11)

include_directories(inc)

add_executable(server
        src/server.c
        src/errExit.c
)

add_executable(client
        src/client.c
        src/errExit.c
)
