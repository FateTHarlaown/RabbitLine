cmake_minimum_required(VERSION 3.7)

project(coroutline)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -g -fno-strict-aliasing -O2 -Wall -export-dynamic -Wall -pipe  -D_GNU_SOURCE -D_REENTRANT -fPIC -Wno-deprecated -m64)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE "Debug")

enable_language(C ASM)

set(SOURCE_FILES
        coroutline.cpp
        poller.cpp
        poller.h
        timers.h
        timers.cpp
        callbacks.h
        syshook.cpp channel.cpp channel.h)

add_library(co_static STATIC ${SOURCE_FILES})
set_target_properties(co_static PROPERTIES OUTPUT_NAME co)

link_libraries(pthread dl)

add_executable(coroutline example_basic.cpp)
target_link_libraries(coroutline co_static dl)

add_executable(example_echo_server example_echo_server.cpp)
target_link_libraries(example_echo_server co_static dl)
