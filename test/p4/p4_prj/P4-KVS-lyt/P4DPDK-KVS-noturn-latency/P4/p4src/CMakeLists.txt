set(CMAKE_INSTALL_PREFIX $ENV{SDE_INSTALL})
set(CMAKE_MODULE_PATH    $ENV{SDE}/cmake)

set(P4_NAME P4Lock)
set(P4_PATH ${CMAKE_CURRENT_SOURCE_DIR}/test.p4)

set(P4_BUILD_DIRECTORY p4build)

add_subdirectory($ENV{SDE}/p4studio/ ${P4_BUILD_DIRECTORY} EXCLUDE_FROM_ALL)

add_custom_target(p4_build # for SDE version after 9.6.0
                  COMMAND make clean && make install
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${P4_BUILD_DIRECTORY})