cmake_minimum_required(VERSION 2.6)

project (hash-tailer)

SET(CMAKE_CXX_COMPILER g++52)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14 -g -O3 -m64 -mavx -maes")
set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES hash-tailer.cpp)
add_executable(hash-tailer ${SOURCE_FILES})