cmd_simple_socket.o = gcc -Wp,-MD,./.simple_socket.o.d.tmp  -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C -DRTE_MACHINE_CPUFLAG_AVX2  -I/home/user/hz/NetPB/client_dpdk/build/include -I/home/user/dpdk/dpdk-stable-16.11.1/x86_64-native-linuxapp-gcc/include -include /home/user/dpdk/dpdk-stable-16.11.1/x86_64-native-linuxapp-gcc/include/rte_config.h -O3     -o simple_socket.o -c /home/user/hz/NetPB/client_dpdk/simple_socket.c 