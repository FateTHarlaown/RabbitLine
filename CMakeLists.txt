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
        syshook.cpp
        channel.cpp
        channel.h
        rabbitline.h
        rabbitline.cpp
        example_timer.cpp cond.cpp cond.h)

add_library(co_static STATIC ${SOURCE_FILES} rabbitline.cpp)
set_target_properties(co_static PROPERTIES OUTPUT_NAME co)

link_libraries(pthread dl)

add_executable(example_basic example_basic.cpp)
target_link_libraries(example_basic co_static dl)

add_executable(example_echo_server example_echo_server.cpp)
target_link_libraries(example_echo_server co_static dl)

add_executable(example_timer example_timer.cpp)
target_link_libraries(example_timer co_static dl)

add_executable(example_cond example_cond.cpp)
target_link_libraries(example_cond co_static dl)
