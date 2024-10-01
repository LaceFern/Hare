
# find include path and link path
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_library(dpdk_server_stat_lib
        stats_server.cpp ${PROJECT_SOURCE_DIR}/include/stats_server.h
)

target_include_directories(dpdk_server_stat_lib PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/extern/ctrl_lib/include
)

target_compile_options(dpdk_server_stat_lib PRIVATE -O2)

target_compile_options(dpdk_server_stat_lib PRIVATE ${DPDK_CFLAGS})

target_link_libraries(dpdk_server_stat_lib
        dpdk_lib
        dpdk_ctrl_lib
)