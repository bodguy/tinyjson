cmake_minimum_required(VERSION 3.10)
project(tinyjson)

set(CMAKE_CXX_STANDARD 14)

#SET(GCC_COVERAGE_COMPILE_FLAGS "-finline-limit=10")
#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

add_executable(tinyjson benchmark/main.cpp tinyjson.h benchmark/utils.h)