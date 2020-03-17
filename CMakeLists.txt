cmake_minimum_required(VERSION 3.15)
project(tinyjson)

set(CMAKE_CXX_STANDARD 14)

add_library(tinyjson INTERFACE)
target_sources(tinyjson INTERFACE tinyjson.h)

add_executable(benchmark)
target_sources(benchmark
	PUBLIC
	benchmark/main.cpp
	benchmark/utils.h
	)
target_link_libraries(benchmark PRIVATE tinyjson)
target_include_directories(benchmark PRIVATE ${CMAKE_SOURCE_DIR})
