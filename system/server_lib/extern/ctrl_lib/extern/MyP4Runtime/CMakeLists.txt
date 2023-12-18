cmake_minimum_required(VERSION 3.7)
project(MyP4Runtime)

set(CMAKE_CXX_STANDARD 14)

message(STATUS "SDE_INSTALL: " $ENV{SDE_INSTALL})

add_library(MyP4Runtime p4_program.cpp p4_program.h p4_table.cpp p4_table.h)

target_include_directories(MyP4Runtime PUBLIC $ENV{SDE_INSTALL}/include)

target_link_directories(MyP4Runtime PUBLIC $ENV{SDE_INSTALL}/lib)

target_link_libraries(MyP4Runtime
                      pthread
                      driver
                      target_sys
                      target_utils
                      )