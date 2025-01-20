
# find include path and link path
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_library(dpdk_ctrl_lib
        general_ctrl.cpp ${PROJECT_SOURCE_DIR}/include/general_ctrl.h
        dpdk_ctrl.cpp ${PROJECT_SOURCE_DIR}/include/dpdk_ctrl.h
)

target_include_directories(dpdk_ctrl_lib PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/extern/dpdk_lib/include
)

target_compile_options(dpdk_ctrl_lib PRIVATE -O2)

target_compile_options(dpdk_ctrl_lib PRIVATE ${DPDK_CFLAGS})

target_link_libraries(dpdk_ctrl_lib
        dpdk_lib
)

if(NOT $ENV{SDE_INSTALL} STREQUAL "")

    message(STATUS "p4_ctrl_lib available")

    add_library(p4_ctrl_lib
            general_ctrl.cpp ${PROJECT_SOURCE_DIR}/include/general_ctrl.h
            p4_ctrl.cpp ${PROJECT_SOURCE_DIR}/include/p4_ctrl.h
    )

    target_include_directories(p4_ctrl_lib PUBLIC
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/extern/MyP4Runtime
    )

    target_compile_options(p4_ctrl_lib PRIVATE -O2)

    target_link_libraries(p4_ctrl_lib PUBLIC
            MyP4Runtime
    )

endif()



