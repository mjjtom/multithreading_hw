cmake_minimum_required(VERSION 3.16)
project(mutex_futex CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(mutex_tests mutex_test.cpp)
target_link_libraries(mutex_tests PRIVATE GTest::gtest_main Threads::Threads)

include(GoogleTest)
gtest_discover_tests(mutex_tests)