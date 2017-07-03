cmake_minimum_required(VERSION 3.7)
project(TP1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(SOURCE_FILES main.c tp1opt.c tp1opt.h)
add_executable(TP1 ${SOURCE_FILES})