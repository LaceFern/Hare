find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

#add_executable(controller src/main.cpp
#        include/ctrl.h
#        include/dpdk_basic.h
#        include/dpdk_udp.h
#        include/dpdk_clib.h
#        src/ctrl.cpp
#        src/ctrl.cpp
#        src/dpdk_basic.cpp
#        src/dpdk_udp.cpp
#        src/dpdk_basic.cpp
#        src/ctrl.cpp)

add_library(dpdk_lib
        dpdk_basic.cpp # ../include/dpdk_basic.h
        dpdk_udp.cpp # ../include/dpdk_udp.h
        # ../include/dpdk_clib.h
)

target_include_directories(dpdk_lib PRIVATE ../include)

target_compile_options(dpdk_lib PRIVATE -O2)

target_compile_options(dpdk_lib PUBLIC ${DPDK_CFLAGS})
target_link_libraries(dpdk_lib PUBLIC
        ${DPDK_LDFLAGS}
)