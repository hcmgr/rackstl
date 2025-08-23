cmake_minimum_required(VERSION 3.14)
project(mystl)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/src)

# gmock stuff
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

## EXECUTABLE - tests
file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/test/*.cpp")
add_executable(test ${TEST_SOURCES})
target_link_libraries(test PRIVATE GTest::gtest_main)

## EXECUTABLE - benchmark
file(GLOB BENCH_SOURCES "${CMAKE_SOURCE_DIR}/bench/*.cpp")
add_executable(bench ${BENCH_SOURCES})

