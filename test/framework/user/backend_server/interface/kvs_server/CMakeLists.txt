
# find include path and link path
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_executable(KVS_server
        main.cpp
        kvs_app.cpp
        kvs_app.h
#        ../../include/stats_server.h
#        ../../src/stats_server.cpp
#        ../../include/general_server.h
)

target_include_directories(KVS_server PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/extern/ctrl_lib/include
        ${PROJECT_SOURCE_DIR}/extern/ctrl_lib/extern/dpdk_lib/include
)

target_link_libraries(KVS_server dpdk_ctrl_lib dpdk_server_stat_lib)

target_compile_options(KVS_server PRIVATE -O2)

target_compile_options(KVS_server PRIVATE ${DPDK_CFLAGS})
