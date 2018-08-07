cmake_minimum_required(VERSION 3.7)
project(coroutline)

set(CMAKE_CXX_STANDARD 11)

link_libraries(pthread)

set(SOURCE_FILES main.cpp coroutline.cpp poller.cpp poller.h timers.h timers.cpp callbacks.h)
add_executable(coroutline ${SOURCE_FILES})