add_executable(controller main.cpp)

# find include path and link path
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

target_include_directories(controller PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/extern/dpdk_lib/include
        ${PROJECT_SOURCE_DIR}/extern/MyP4Runtime
)

target_link_libraries(controller p4_ctrl_lib dpdk_ctrl_lib dpdk_lib)

target_compile_options(controller PRIVATE ${DPDK_CFLAGS})