cmake_minimum_required(VERSION 3.7)

project(coroutline)

set(CMAKE_CXX_FLAGS -D_GNU_SOURCE)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE "Debug")

link_libraries(pthread dl)

set(SOURCE_FILES main.cpp coroutline.cpp poller.cpp poller.h timers.h timers.cpp callbacks.h syshook.cpp)
add_executable(coroutline ${SOURCE_FILES})