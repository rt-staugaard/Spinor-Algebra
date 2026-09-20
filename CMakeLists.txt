cmake_minimum_required(VERSION 3.14)

project(spinor_algebra VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_library(spinor_algebra INTERFACE)
target_include_directories(spinor_algebra INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(spinor_tests tests/spinor_test.cpp)
target_link_libraries(spinor_tests PRIVATE spinor_algebra GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(spinor_tests)