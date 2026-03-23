cmake_minimum_required(VERSION 3.16)
project(IpcTask CXX)

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

add_executable(ipc_test test.cpp)
target_include_directories(ipc_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ipc_test PRIVATE GTest::gtest GTest::gtest_main Threads::Threads rt)
enable_testing()
add_test(NAME IpcTests COMMAND ipc_test)

add_executable(producer producer.cpp)
target_include_directories(producer PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(producer PRIVATE Threads::Threads rt)

add_executable(consumer consumer.cpp)
target_include_directories(consumer PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(consumer PRIVATE Threads::Threads rt)