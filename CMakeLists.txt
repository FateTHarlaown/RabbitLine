cmake_minimum_required(VERSION 3.7)

project(coroutline)

set(CMAKE_CXX_FLAGS -D_GNU_SOURCE)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE "Debug")


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

add_executable(coroutline main.cpp)
target_link_libraries(coroutline co_static)