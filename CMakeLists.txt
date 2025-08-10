cmake_minimum_required(VERSION 3.12)
project(Pthread_SHA256Project C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

include_directories(inc)

add_executable(server
        src/server.c
        src/errExit.c
)

add_executable(client
        src/client.c
        src/errExit.c
)

target_link_libraries(server pthread)
target_link_libraries(client pthread)