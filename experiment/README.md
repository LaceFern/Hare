## How to run Hare experiments

### Switch
SDE: 9.6.0 (if there is no $SDE_INSTALL/bin/bf_kpkt_mod_load or $SDE_INSTALL, using 9.9.0)

DPDK：21.05

KVS：./p4/p4_prj/P4-KVS-lyt/P4DPDK-KVS-noturn-latency

Lock：./p4/p4_prj/P4-Lock-lyt/P4-Lock-testlatency

#### Step 1. check SDE version

```bash
sudo update-alternatives --config bf-sde
```

#### Step 2. config environment variables

```bash
# SDE
export SDE=/root/Software/bf-sde
export SDE_INSTALL=$SDE/install
export PATH=$PATH:$SDE_INSTALL/bin
# export LD_LIBRARY_PATH=/usr/local/lib:$SDE_INSTALL/lib
```

#### Step 3. config PISA architecture

```bash
sudo -i
$SDE_INSTALL/bin/bf_kpkt_mod_load $SDE_INSTALL # load pisa driver
ifconfig enp6s0 up # bring up virtual network interface
```

#### Step 4. config dpdk-21.05

```bash
sudo -i
modprobe uio_pci_generic # load kernel module
dpdk-devbind.py --status # check status 
dpdk-devbind.py --bind=uio_pci_generic 04:00.0 # bind network interface to uio module
mount -t hugetlbfs pagesize=1GB /mnt/huge # mount huge page
```

#### Step 5. config parameters

parameters in controller/config.h

```c
#define SLOT_SIZE 32768
#define TOPK 800
#define HOTOBJ_IN_ONEPKT 20
```

parameters in p4src/config.p4

```c
#define SLOT_SIZE 32768
#define CPU_CONTROLLER  24
```

#### Step 6. compile and run

```bash
mkdir build && cd build
cmake ../
cd p4src/p4build && make && make install && cd ../../
cd controller && make && cd ../
#
cd controller
./hello_bfrt -l 0
```
#### Debug

```bash
$SDE/run_bfshell.sh
ucli
pm show
```

************************************************************************************************
### Backend server

KVS：./server/server_code_kvs_noturn_testlatency/KVS_dpdk_server_code_amax7

Lock：./server/server_code_lock_noturn_testlatency/ver_UPD

#### Step 1. config parameters

./include/cache_upd_ctrl.h

```c
#define IP_CTRL               "10.0.0.3"
#define MAC_CTRL               {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18}

#define MAX_CACHE_SIZE  32768
#define TOPK 800

#define NUM_BACKENDNODE 1
char* ip_backendNode_arr[NUM_BACKENDNODE] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};

//if in lock system, mention the following variables
#define NUM_CLIENT 3
char* ip_client_arr[NUM_CLIENT] = {
    "10.0.0.1",
    "10.0.0.2",
    "10.0.0.5"
};
struct rte_ether_addr mac_client_arr[NUM_CLIENT] = {
    {.addr_bytes = {0xb8, 0x59, 0x9f, 0xe9, 0x6b, 0x1c}},
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x48, 0x38}},
    {.addr_bytes = {0x04, 0x3f, 0x72, 0xde, 0xba, 0x44}}
};
```

./include/util.h

```c
#define IP_LOCAL                "10.0.0.4"
#define MAC_LOCAL               {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}
```

#### Step 2. compile & run

```bash
make
sudo ./build/netlock_server_kvs -l 0-11,24-35 # KVS
# or
make
sudo ./build/netlock_server_lock -l 0-11,24-35 # Lock
```

************************************************************************************************

### Client

KVS：./client/netCache_huawei/client_speedlimit

Lock：./client/netLock_huawei_latency/client

#### Step 1. config parameters

./include/util.h

```c
#define IP_LOCAL                "10.0.0.1"
#define MAC_LOCAL               {0xb8, 0x59, 0x9f, 0xe9, 0x6b, 0x1c} //amax 1
#define CLIENT_PORT             1111 // 1111,9303,17495,25687,33879,42071,50263,58455
#define HC_MASTER               1 // "hc" is short for hotspot change
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

#define BN_NUM            1
char* ip_dst_arr[BN_NUM] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_dst_arr[BN_NUM] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};
```

#### Step 2. 编译&运行

```bash
make
sudo ./build/netlock_client -l 0-10
```

************************************************************************************************
### Control node

KVS：./controller/netlock_huawei/ctrl_code_kvs_noturn_latency

Lock：./controller/netlock_huawei/ctrl_code_lock_noturn_latency

#### Step 1. config parameters

./include/cache_upd_ctrl.h

```c
#define IP_CTRL               "10.0.0.3"
#define MAC_CTRL               {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18}

#define MAX_CACHE_SIZE  32768
#define TOPK 800
#define HOTOBJ_IN_ONEPKT 20

#define NUM_BACKENDNODE 1
char* ip_backendNode_arr[NUM_BACKENDNODE] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};
```

#### Step 2. config & run

```bash
make
sudo ./build/netlock_ctrl -l 0-2
```

************************************************************************************************
### Probe

/home/zxy/nfs/Hare/probe/client_probe_noClientID

#### Step 1. compile & run

mention: the hc_flag in util.h should be are consistent with that in client-related files.

```bash
make
sudo ./build/netlock_probe -l 0-1
```

## How to run microbench about BFRT-MAT-insertion batching 

P4 Project: ./p4/p4_prj/P4-cp_api_batch

```bash
./hello_bfrt -p P4Lock
```

## How to run microbench about switch dp resource

P4 Project: ./p4/p4_prj/P4-dp_resource

compile, run, and use P4i to check resource utilization

## How to run microbench about server with multi Stat threads

Server Project：./server/server_code_kvs_noturn_testlatency_multiStatThread

```bash
make
sudo ./build/netlock_server_kvs -l 0-11,24-35
```

## How to run with twitter trace

Client project：./client/client_speedlimit_w_twitter
