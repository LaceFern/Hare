
This repository is a simple example of Hare's hotspot offloading process. It offloads functions except MAT modification from the switch control plane to an additional server (control server), which is more stable.

## System settings

### Switch

SDE: 9.6.0

DPDK: 21.05

Kernel: Linux p4-switch 4.14.151-OpenNetworkLinux

### Server

DPDK: 21.05

Kernel: Ubuntu 22.04.4 LTS (GNU/Linux 5.15.102 x86_64)

## How to run

### Part 1. Switch

Project location: ./p4/p4_prj/P4-KVS-lyt/P4DPDK-KVS-noturn-latency

#### Step 1. Configurate 

The following macros in controller/config.h need to remain consistent with the switch data plane, the control server, and backend servers.

```c
#define SLOT_SIZE 32768
#define TOPK 600
#define HOTOBJ_IN_ONEPKT 20
```

The following macros in p4src/config.p4 need to remain consistent with the switch control plane, the control server, and backend servers.

```c
#define SLOT_SIZE 32768
#define CPU_CONTROLLER  24 //the port linked by the control server
```

#### Step 2. Compile and run

```bash
mkdir build && cd build
cmake ../
cd p4src/p4build && make && make install && cd ../../
cd controller && make && cd ../
#
cd controller
./hello_bfrt -l 0
```

### Part 2. Backend server

Project location: /home/zxy/nfs/Hare/server/server_code_kvs_noturn_testlatency/KVS_dpdk_server_code_amax7

#### Step 1. Configurate 

The following macros in include/cache_upd_ctrl.h need to remain consistent with the switch control plane, the control server, and backend servers, and to align with the actual situation.

```c
#define IP_CTRL               "10.0.0.3"
#define MAC_CTRL               {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18}

#define MAX_CACHE_SIZE  32768
#define TOPK 600//800//8

#define NUM_BACKENDNODE 1
char* ip_backendNode_arr[NUM_BACKENDNODE] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};
```

The following macros in include/util.h need to align with the actual situation.

```c
#define IP_LOCAL                "10.0.0.4"
#define MAC_LOCAL               {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}
```

#### Step 2. Compile and run

```bash
make
sudo ./build/netlock_server_kvs -l 0-11,24-35
```

### Part 3. Client

Project location: /home/zxy/nfs/Hare/client/netCache_huawei/client_speedlimit

#### Step 1. Configurate 

The following macros in include/util.h need to align with the actual situation.

```c
#define IP_LOCAL                "10.0.0.1"
#define MAC_LOCAL               {0xb8, 0x59, 0x9f, 0xe9, 0x6b, 0x1c} //amax 1
#define CLIENT_PORT             1111 // different client needs different udp port (1111,9303,17495,25687,33879,42071,50263,58455)
#define HC_MASTER               1 // "HC" is short for hotspot change
#define HC_SLAVE_NUM            4
volatile int hc_flag = 0;
char* ip_dst_hc_arr[HC_SLAVE_NUM] = {
    "10.0.0.2",
    "10.0.0.3",
    "10.0.0.5",
    "10.0.0.8"
};
struct rte_ether_addr mac_dst_hc_arr[HC_SLAVE_NUM] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x48, 0x38}},
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18}},
    {.addr_bytes = {0x04, 0x3f, 0x72, 0xde, 0xba, 0x44}},
    {.addr_bytes = {0x0c, 0x42, 0xa1, 0x2b, 0x0d, 0x70}}
};

#define BN_NUM            1 // "BN" is short for backend node (backend server)
char* ip_dst_arr[BN_NUM] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_dst_arr[BN_NUM] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};
```

#### Step 2. Compile and run

```bash
make
sudo ./build/netlock_client -l 0-5
```

### Part 4. Control node

Project location: /home/zxy/nfs/Hare/controller/netlock_huawei/ctrl_code_kvs_noturn_latency

#### Step 1. Configurate

The following macros in include/cache_upd_ctrl.h need to align with the actual situation.

```c
#define IP_CTRL               "10.0.0.3"
#define MAC_CTRL               {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18}

#define MAX_CACHE_SIZE  32768
#define TOPK 600//800//1000 //8
#define HOTOBJ_IN_ONEPKT 20//20//20 //50 //20

#define NUM_BACKENDNODE 1
char* ip_backendNode_arr[NUM_BACKENDNODE] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};
```

#### Step 2. Compile and run

```bash
make
sudo ./build/netlock_ctrl -l 0-2
```
