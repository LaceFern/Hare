cmake_minimum_required(VERSION 3.7)
project(controller)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(extern/dpdk_lib)
add_subdirectory(extern/MyP4Runtime)

add_subdirectory(src)

add_subdirectory(app/controller)