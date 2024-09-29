set(CMAKE_CXX_STANDARD 14)

include_directories($ENV{SDE_INSTALL}/include)
link_directories($ENV{SDE_INSTALL}/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(include)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_executable(hello_bfrt
               my_header.h
               main.cpp
               config.h)

target_compile_options(hello_bfrt PRIVATE ${DPDK_CFLAGS})
target_link_libraries(hello_bfrt
                      ${DPDK_LDFLAGS}
                      MyP4Runtime
                      )