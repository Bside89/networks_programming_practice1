cmake_minimum_required(VERSION 3.7)
project(TP1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

set(GCC_PTHREAD_FLAG "-lpthread")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_PTHREAD_FLAG}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_PTHREAD_FLAG}")

set(CLIENT_SOURCE_FILES client.c lib/tp1opt.c lib/tp1opt.h lib/srvutils.c lib/srvutils.h)
set(SERVER_SOURCE_FILES server.c lib/tp1opt.c lib/tp1opt.h lib/slist.c lib/slist.h lib/srvutils.c lib/srvutils.h)

add_executable(client1 ${CLIENT_SOURCE_FILES})
add_executable(client2 ${CLIENT_SOURCE_FILES})
add_executable(server ${SERVER_SOURCE_FILES})
