cmake_minimum_required(VERSION 3.10)
project(mystl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/src)

## test
file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/test/*.cpp")
add_executable(test ${TEST_SOURCES})

## benchmark
file(GLOB BENCH_SOURCES "${CMAKE_SOURCE_DIR}/bench/*.cpp")
add_executable(bench ${BENCH_SOURCES})

