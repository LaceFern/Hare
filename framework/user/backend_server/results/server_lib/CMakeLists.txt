cmake_minimum_required(VERSION 3.25)
project(KVS_server)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(extern/ctrl_lib)

add_subdirectory(app/kvs_server)

add_subdirectory(src)
