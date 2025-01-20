cmake_minimum_required(VERSION 3.7)
project(dpdk_lib)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(src)

add_subdirectory(tests)