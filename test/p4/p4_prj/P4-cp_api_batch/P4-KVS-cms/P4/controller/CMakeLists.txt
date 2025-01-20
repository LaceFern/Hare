set(CMAKE_CXX_STANDARD 14)

include_directories($ENV{SDE_INSTALL}/include)
link_directories($ENV{SDE_INSTALL}/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(include)

add_executable(hello_bfrt
               my_header.h
               main.cpp
               config.h)

target_link_libraries(hello_bfrt
                      MyP4Runtime
                      )