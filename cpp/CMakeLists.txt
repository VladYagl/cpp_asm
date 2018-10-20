cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "-pthread -lpthread -Wl,--no-as-needed")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall -std=c++11 -pedantic")

add_executable(word_count
        word_count.cpp
        )

add_executable(memcpy
        memcpy.cpp
        other.h
        )

add_executable(trampoline
        trampoline_test.cpp
        )
