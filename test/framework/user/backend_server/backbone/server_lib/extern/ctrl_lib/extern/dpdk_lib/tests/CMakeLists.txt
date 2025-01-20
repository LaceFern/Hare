add_executable(main_test main.cpp)

target_include_directories(main_test PRIVATE ../include)

target_link_libraries(main_test dpdk_lib)