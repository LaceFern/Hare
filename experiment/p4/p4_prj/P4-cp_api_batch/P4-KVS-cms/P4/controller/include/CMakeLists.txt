cmake_minimum_required(VERSION 3.7)
project(MyP4Runtime)

set(CMAKE_CXX_STANDARD 14)

include_directories($ENV{SDE_INSTALL}/include)
link_directories($ENV{SDE_INSTALL}/lib)

add_library(MyP4Runtime p4_program.cpp p4_program.h p4_table.cpp p4_table.h)

target_link_libraries(MyP4Runtime
                      -lpthread
                      -ldriver
                      -lbfutils
#                      -lbfsys
#                      -lbf_switchd_lib
#                      :bfshell_plugin_bf_rt.so
#                      :bfshell_plugin_bf_switchapi.so
#                      :bfshell_plugin_clish.so
#                      :bfshell_plugin_debug.so
#                      :bfshell_plugin_pipemgr.so
#                      -ltofinopdfixed_thrift
#                      -lacctonbf_driver
#                      -lplatform_thrift
#                      -lbf_switch
#                      -lbfdiags
#                      -lcrafter
#                      -ldiag_thrift
#                      -ldru_sim
#                      -lplatform_thrift
#                      -lpltfm_driver
#                      -lpltfm_mgr
#                      -lsai
#                      -ltcl_server
                      )