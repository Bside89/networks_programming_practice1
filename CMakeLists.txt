cmake_minimum_required(VERSION 3.7)
project(TP1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

set(GCC_PTHREAD_FLAG "-lpthread")

#set(SOURCE_FILES server.c client.c tp1opt.c tp1opt.h)
#add_executable(TP1 ${SOURCE_FILES})

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_PTHREAD_FLAG}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_PTHREAD_FLAG}")

add_executable(client client.c tp1opt.c tp1opt.h)
add_executable(server server.c tp1opt.c tp1opt.h)
