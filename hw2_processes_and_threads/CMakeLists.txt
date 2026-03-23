cmake_minimum_required(VERSION 3.16)
project(ApplyFunctionTask CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

find_package(GTest QUIET)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(apply_function_test apply_function_test.cpp)
target_include_directories(apply_function_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(apply_function_test
    PRIVATE GTest::gtest GTest::gtest_main Threads::Threads)
enable_testing()
add_test(NAME ApplyFunctionTests COMMAND apply_function_test)

find_package(benchmark QUIET)
if(NOT benchmark_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.8.3
  )
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googlebenchmark)
endif()

add_executable(apply_function_bench apply_function_bench.cpp)
target_include_directories(apply_function_bench PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(apply_function_bench
    PRIVATE benchmark::benchmark Threads::Threads)
